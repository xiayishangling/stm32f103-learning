# 6.18 综合项目 FreeRTOS 重构 — 互斥锁 + 计数信号量

> **芯片**：STM32F103C8T6 | **环境**：CubeMX + VSCode + Keil  
> **主题**：裸机综合项目→RTOS重构、互斥锁、计数信号量、6任务管道

---

## 一、核心代码

### 1. 全局状态 + 互斥锁保护 OLED

```c
// 结构体集中管理全局状态
typedef struct {
    LED_State    led;
    OLED_State   oled;
    W25Q64_State w25q64;
} AppState;

volatile AppState g_state = { LED_State_OFF, OLED_State_Display3, W25Q64_State_Save };

// LED 任务：互斥锁保护 OLED 刷新
void vIP_LEDTask(void *argument)
{
    for (;;) {
        USARTMessage msg;
        if (osMessageQueueGet(USART_QueueHandle, &msg, 0, osWaitForever) == osOK) {
            g_state.led  = msg.led_state;
            g_state.oled = msg.oled_state;

            // OLED 是共享硬件 → 上锁
            osMutexAcquire(General_MutexHandle, osWaitForever);
            OLED_Show_LED_Status(msg.led_state);  // 刷新屏幕
            osMutexRelease(General_MutexHandle);
        }
    }
}
```

### 2. 计数信号量：串口接收不丢数据

```c
// 中断回调：每收一个字节 → Release（计数+1）
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        osSemaphoreRelease(USART_CountingSemHandle);
        HAL_UART_Receive_IT(&huart1, &rxData, 1);  // 立即重启接收
    }
}

// 串口任务：Acquire（计数-1）→ 处理，直到计数归零
void vIP_USARTTask(void *argument)
{
    UART1_Receive_Start();
    char cmdbuff[8]; uint16_t idx = 0;
    for (;;) {
        osSemaphoreAcquire(USART_CountingSemHandle, osWaitForever);
        uint8_t byte = rxData;
        if (byte == '\r' || byte == '\n') {
            if (idx > 0) {
                cmdbuff[idx] = '\0';
                if      (strcmp(cmdbuff, "1") == 0) usart.led_state = LED_State_ON;
                else if (strcmp(cmdbuff, "0") == 0) usart.led_state = LED_State_OFF;
                else if (strcmp(cmdbuff, "2") == 0) usart.led_state = LED_State_Flicker;
                osMessageQueuePut(USART_QueueHandle, &usart, 0, 0);
                idx = 0;
            }
        } else { cmdbuff[idx++] = byte; }
    }
}
```

### 3. 按键 → 队列 → W25Q64 任务

```c
// 中断回调：两个按键共用，极简
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == PA1_KEY1_Pin || GPIO_Pin == PA2_KEY2_Pin)
        osSemaphoreRelease(KEY_BinarySemHandle);  // 只释放，不处理
}

// 按键任务：消抖 → 识别 → 入队
void vIP_KEYTask(void *argument)
{
    for (;;) {
        osSemaphoreAcquire(KEY_BinarySemHandle, osWaitForever);
        vTaskDelay(pdMS_TO_TICKS(20));  // 软件消抖
        KEYMessage msg;
        if (!HAL_GPIO_ReadPin(PA1_KEY1_Port, PA1_KEY1_Pin))  msg.w25q64_state = W25Q64_State_Save;
        else if (!HAL_GPIO_ReadPin(PA2_KEY2_Port, PA2_KEY2_Pin)) msg.w25q64_state = W25Q64_State_Recovery;
        else continue;
        osMessageQueuePut(KEY_QueueHandle, &msg, 0, 0);
    }
}

// W25Q64 任务：出队 → SPI 保存/读取 → 更新全局状态
void vIP_W25Q64Task(void *argument)
{
    for (;;) {
        KEYMessage msg;
        if (osMessageQueueGet(KEY_QueueHandle, &msg, 0, osWaitForever) == osOK) {
            if (msg.w25q64_state == W25Q64_State_Save) {
                uint8_t cur = HAL_GPIO_ReadPin(LED_Port, LED_Pin);
                W25Q64_Save_State(cur ? 0x01 : 0x00);
            } else {
                uint8_t rcvy = W25Q64_Read_ME();
                if (rcvy == 0x01)      { g_state.led = LED_State_ON;  g_state.oled = OLED_State_Display1; }
                else if (rcvy == 0x00) { g_state.led = LED_State_OFF; g_state.oled = OLED_State_Display2; }
            }
        }
        osDelay(10);
    }
}
```

---

## 二、核心知识点

### 1. 互斥锁 vs 二进制信号量

| 特性 | 互斥锁 | 二进制信号量 |
|------|--------|-------------|
| 用途 | 保护**共享资源**互斥 | **事件同步**通知 |
| 所有权 | **谁锁谁解** | 任意任务可 Give/Take |
| 优先级继承 | ✅ 有 | ❌ 无 |
| 典型场景 | OLED/SPI 共享硬件 | 中断通知任务 |

### 2. 计数信号量：比二进制更可靠

```
二进制：只能记录"有过事件" → 任务来不及处理 = 丢失
计数：  每来一个事件计数+1 → 任务逐个处理，不丢
```

> 串口连续接收是最典型的计数信号量场景

### 3. 任务管道设计（6 任务）

```
中断(极短)          任务(业务逻辑)
  │                    │
  ├ USART中断 ──Sem──→ vIP_USARTTask ──Queue──→ vIP_LEDTask
  │                    解析命令               │ LED+OLED
  │                                           │
  ├ KEY中断 ───Sem──→ vIP_KEYTask ──Queue──→ vIP_W25Q64Task
  │                   消抖+识别             │ SPI Flash
  │                                         │
  │                    vIP_OLEDTask           vIP_PA11Flick
  │                    开屏动画               蓝色小灯常闪
```

每个任务只做一件事，队列解耦，修改一个不影响其他。

### 4. 全局状态安全策略

```c
// 所有读写 g_state 的地方统一加锁
osMutexAcquire(General_MutexHandle, osWaitForever);
g_state.led = new_state;
osMutexRelease(General_MutexHandle);
```

> 裸机时代用 `volatile` + 状态机，RTOS 时代用互斥锁——本质相同，形式升级

### 5. 中断回调"快进快出"规范（最终版）

```c
void HAL_GPIO_EXTI_Callback(uint16_t pin) {
    if (pin == KEY1 || pin == KEY2)
        osSemaphoreRelease(KEY_BinarySemHandle);  // 仅此一行，无延时、无打印
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *h) {
    if (h->Instance == USART1) {
        osSemaphoreRelease(USART_CountingSemHandle);  // 仅此一行
        HAL_UART_Receive_IT(&huart1, &rxData, 1);
    }
}
```

---

## 三、心得

- 学习 FreeRTOS 第三天就完成了裸机第一周综合项目的完整重构——从状态机到多任务管道，思维迁移非常顺畅
- 互斥锁解决了一个裸机时代从未想过的问题：**多个任务同时抢 OLED 怎么办**
- 计数信号量是串口接收的正确答案——二进制只能记录"来过"，计数能记住"来了几次"
- 6 个任务各司其职 + 队列解耦，这个架构已经接近实际产品的设计风格