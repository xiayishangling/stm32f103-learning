# 6.16 FreeRTOS 队列实战 — 中断→队列→任务

> **芯片**：STM32F103C8T6 | **环境**：CubeMX + VSCode + Keil  
> **主题**：SRAM 五段分区、命名规范、任务四态、队列传值 vs 传指针

---

## 一、核心代码

### 1. 按键消抖（RTOS 非阻塞版）

```c
// 静态局部变量保存计数器+按压状态，10ms 扫描一次
uint8_t isClickBtn(void)
{
    static uint32_t count = 0;
    static uint32_t pressed = 0;

    if (IS_KEY_PRESSED() && !pressed) {
        count++;
        if (count >= KEY_DEBOUNCE_COUNT && IS_KEY_PRESSED()) {
            pressed = 1;
            return 1;   // 确认一次击键
        }
    }
    if (!IS_KEY_PRESSED()) {
        count = 0;
        pressed = 0;     // 松开后全部清零
    }
    return 0;
}
```

### 2. 队列传指针：按键 → LED 控制

```c
typedef enum  { LEDColor_Red, LEDColor_Blue } LED_Color;
typedef enum  { LEDState_ON = 1, LEDState_OFF = 0 } LED_State;
typedef struct { LED_Color color; LED_State state; } LED_Message;

// 按键任务：动态分配消息 → 队列
void vBtnTask(void *argument)
{
    LED_State state = LEDState_OFF;
    for (;;) {
        if (isClickBtn()) {
            state = (state == LEDState_OFF) ? LEDState_ON : LEDState_OFF;
            LED_Message *msg = pvPortMalloc(sizeof(LED_Message));
            if (msg) {
                msg->color = LEDColor_Red;
                msg->state = state;
                if (osMessageQueuePut(LEDQueueHandle, &msg, 0, osWaitForever) != osOK)
                    vPortFree(msg);   // 队列满则释放，避免泄漏
            }
        }
        osDelay(10);
    }
}

// LED 任务：取消息 → 写 GPIO → 释放内存
void vLEDTask(void *argument)
{
    for (;;) {
        LED_Message *msg;
        if (osMessageQueueGet(LEDQueueHandle, &msg, 0, 0) == osOK) {
            GPIO_WritePin(..., msg->state ? SET : RESET);
            vPortFree(msg);   // 处理完必须释放！
        }
    }
}
```

### 3. 中断回调 → 队列 → 命令任务

```c
uint8_t rxData;

// 启动中断接收
void UART1_Receive_Start(void)
    { HAL_UART_Receive_IT(&huart1, &rxData, 1); }

// 接收完成回调 → 只投递队列，立即重启接收
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        osMessageQueuePut(CommandQueueHandle, &rxData, 0, 0);
        HAL_UART_Receive_IT(&huart1, &rxData, 1);
    }
}

// 命令任务：从队列取字节 → 解析执行
void vCommandTask(void *argument)
{
    UART1_Receive_Start();
    uint8_t byte;
    for (;;) {
        if (osMessageQueueGet(CommandQueueHandle, &byte, 0, osWaitForever) == osOK) {
            switch (byte) {
                case '1': LED1_Toggle(); break;
                case '2': LED2_ON();     break;
                case '3': LED1_OFF(); LED2_OFF(); break;  // ⚠️ 必须加 break
            }
        }
    }
}
```

---

## 二、核心知识点

### 1. SRAM 五段分区

| 段 | 存储内容 | 增长方向 | 说明 |
|----|----------|----------|------|
| `.data` | 非零初值全局/静态变量 | — | 初值在 Flash，启动时复制 |
| `.bss` | 零初值/无初值全局/静态 | — | 启动自动清零，不占 Flash |
| `.heap` | `malloc` 动态分配 | ↑ 向高地址 | FreeRTOS 内核对象在此 |
| (空闲) | — | — | 堆栈相撞 = 内存溢出 |
| `.stack` | 局部变量/参数/返回地址 | ↓ 向低地址 | 每任务独立栈 |

### 2. FreeRTOS 命名规范

| 前缀 | 类型 | 示例 |
|------|------|------|
| `c` | char | `cCount` |
| `s` | short | `sValue` |
| `l` | long | `lGlobalVar` |
| `x` | BaseType_t/句柄 | `xQueue` |
| `e` | enum | `eState` |
| `p` | 指针 | `pxTask` |
| `u` | unsigned | `uxPriority` |
| 叠加 | `puc` = uint8_t* | `pucBuffer` |

函数：`返回值前缀 + 模块名 + 功能名`（如 `xTaskCreate`）  
静态函数：`prv` 开头（private）  
宏：`文件前缀小写_大写主体`（如 `pdTRUE`、`configMAX_PRIORITIES`）

### 3. 任务四态流转

```
创建 → 就绪 ⇄ 运行 → 阻塞（主动等）→ 就绪
                ↘ 挂起（被动冻结）→ 恢复 → 就绪
```

| 状态 | 特点 | 进入方式 |
|------|------|----------|
| 就绪 | 万事俱备，等 CPU | 创建/阻塞到期/恢复 |
| 运行 | 正在执行（单核唯一） | 调度器选中 |
| 阻塞 | 主动休眠，不占 CPU | `vTaskDelay`/等队列 |
| 挂起 | 被动冻结，需手动恢复 | `vTaskSuspend` |

### 4. 队列传值 vs 传指针

| 方式 | 适用 | 注意 |
|------|------|------|
| **按值** | 小数据（字节/整型） | 直接拷贝，简单安全 |
| **按指针** | 结构体/大块数据 | 用 `pvPortMalloc`，接收方必须 `vPortFree` |

> ⚠️ **常见错误**：传局部变量地址。局部变量出作用域即销毁，接收方读到野指针。

### 5. 中断→队列→任务 经典模式

```
ISR（极短）          任务（耗时处理）
  │                    │
  ├ 收数据             │
  ├ QueuePut ─────────→ ├ QueueGet
  └ 重启中断接收       └ 解析/执行
```

中断只做两件事：**收数据 + 投队列**。这是 RTOS 实时性的根基。

---

## 三、心得

- SRAM 五段分区是理解"栈溢出""内存泄漏"的前提——面试必问
- FreeRTOS 命名规范看似繁琐，实则是读源码的钥匙——`prv`、`pd`、`config` 见名知源
- 队列传指针时 `pvPortMalloc` → `vPortFree` 成对出现，漏一个就是内存泄漏
- 中断回调里只投队列、不处理业务，是 RTOS 与裸机最大的思维转变