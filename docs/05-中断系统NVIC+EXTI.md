# 6.5 中断系统 — NVIC + EXTI

> **芯片**：STM32F103C8T6 | **环境**：VSCode + Keil + Keil Assistant  
> **主题**：NVIC 优先级分组、USART 中断、EXTI 外部中断、轮询与中断冲突

---

## 一、核心代码

### 1. NVIC 配置 + USART 中断

```c
void init_NVIC(void)
{
    // 不需要开时钟，NVIC 属于 Cortex-M 内核
    NVIC_InitTypeDef nvic = {0};
    nvic.NVIC_IRQChannel                   = USART1_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 0;   // 抢占优先级 0
    nvic.NVIC_IRQChannelSubPriority        = 0;   // 子优先级 0
    nvic.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nvic);
}

// USART 初始化中打开 RXNE 中断
USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
```

### 2. USART 中断服务函数

```c
void USART1_IRQHandler(void)
{
    if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET)
    {
        uint16_t ch = USART_ReceiveData(USART1);
        if (ch == '1')      Flicker_time = 500;   // 慢闪
        else if (ch == '2') Flicker_time = 50;    // 快闪
        else if (ch == '0') {
            // 阻塞等待按键（仅演示，实际应避免在 ISR 中死等）
            while (1) {
                if (KEY_PRESSED) { /* 消抖+释放 */ break; }
            }
        }
    }
}

// 主循环：LED 以 Flicker_time 周期闪烁，串口中断动态改速度
void Process_NVIC_Control_LedFlicker(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  // 全局只调一次
    init_NVIC_PC13_Gpio(); init_NVIC_Usart_Gpio();
    init_NVIC_Usart(); init_NVIC();
    while (1) { LED_flicker(); }
}
```

### 3. EXTI 配置（上升沿触发）

```c
void init_EXTI(void)
{
    // ① 绑定 GPIO 引脚到 EXTI 线
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource1);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource2);

    // ② 配置 NVIC（EXTI1 和 EXTI2 各有独立 IRQ 通道）
    NVIC_InitTypeDef nvic = {0};
    nvic.NVIC_IRQChannelPreemptionPriority = 0;
    nvic.NVIC_IRQChannelSubPriority        = 0;
    nvic.NVIC_IRQChannelCmd                = ENABLE;

    nvic.NVIC_IRQChannel = EXTI1_IRQn;  NVIC_Init(&nvic);
    nvic.NVIC_IRQChannel = EXTI2_IRQn;  NVIC_Init(&nvic);

    // ③ 配置 EXTI
    EXTI_InitTypeDef exti = {0};
    exti.EXTI_Line    = EXTI_Line1 | EXTI_Line2;  // 位掩码，可 | 合并
    exti.EXTI_Mode    = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = EXTI_Trigger_Rising;       // 松手触发
    exti.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti);
}
```

### 4. EXTI 中断服务函数

```c
void EXTI1_IRQHandler(void)
{
    if (EXTI_GetFlagStatus(EXTI_Line1) == SET) {
        EXTI_ClearFlag(EXTI_Line1);                     // 先清标志
        GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);   // 松手亮灯
    }
}

void EXTI2_IRQHandler(void)
{
    if (EXTI_GetFlagStatus(EXTI_Line2) == SET) {
        EXTI_ClearFlag(EXTI_Line2);
        GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);     // 松手灭灯
    }
}
```

---

## 二、核心知识点

### 1. NVIC 优先级体系

```
NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
```

| 分组 | 抢占位数 | 子优先级位数 | 抢占范围 | 子范围 |
|------|----------|--------------|----------|--------|
| 0 | 0 | 4 | — | 0~15 |
| 1 | 1 | 3 | 0~1 | 0~7 |
| **2** | **2** | **2** | **0~3** | **0~3** |
| 3 | 3 | 1 | 0~7 | 0~1 |
| 4 | 4 | 0 | 0~15 | — |

**仲裁规则**：
1. 抢占优先级数值小的先执行
2. 抢占相同 → 比子优先级
3. 都相同 → 按 IRQ 编号（小的先响应）
4. 高抢占级可**嵌套打断**低抢占级，子优先级不能打断同抢占级

> `NVIC_PriorityGroupConfig()` 全局只调一次，在所有中断初始化之前

### 2. IRQn（索引）vs 位掩码

| 类型 | 可否 `|` 合并 | 例子 |
|------|:--:|------|
| **IRQn**（中断通道号） | ❌ | `USART1_IRQn` = 37，是索引不是位 |
| **GPIO_Pin / EXTI_Line** | ✅ | `EXTI_Line1 \| EXTI_Line2` = 0x00006 |

> 配置多个 NVIC 通道时需多次调用 `NVIC_Init()`，不能用 `|` 合并 IRQn

### 3. NVIC vs GPIO 结构体差异

| 外设 | 结构体 | 原因 |
|------|--------|------|
| **GPIO / USART / SPI** | 有预设全局结构体 | ST 在库中定义了 `GPIOA`、`USART1` 等 |
| **NVIC** | 需用户自行创建 | 遵循 ARM CMSIS 标准，只提供类型定义 |

### 4. EXTI 配置完整步骤

```
① 开 AFIO 时钟
② GPIO_EXTILineConfig(端口, 引脚源)   — 绑定 GPIO 到 EXTI 线
③ NVIC_Init 配置对应 IRQ 通道
④ EXTI_Init（Line/Mode/Trigger/Cmd）
⑤ ISR 中先 EXTI_GetFlagStatus 确认，再 EXTI_ClearFlag 清除
```

### 5. EXTI 线号与 NVIC 通道映射

| EXTI 线 | IRQ 通道 | 说明 |
|---------|----------|------|
| Line0~4 | `EXTI0_IRQn` ~ `EXTI4_IRQn` | 各自独立 |
| Line5~9 | `EXTI9_5_IRQn` | **5 条线共用一个通道** |
| Line10~15 | `EXTI15_10_IRQn` | 6 条线共用一个通道 |

### 6. 轮询与中断冲突（重要！）

```c
// ❌ 错误：主循环轮询 + EXTI 中断同时控同一个 LED
while (1) {
    EXTI_Button_Control_PC13();  // 轮询：按下立刻变
}
// ISR 中松手又变一次 → 行为混乱

// ✅ 正确：只保留一个决策源
while (1) {
    // 注释掉轮询，纯中断控制
}
```

> 同一资源只能有一个"老板"——要么轮询、要么中断，或用标志位通信

### 7. 硬件流控（RTS/CTS）

| 类型 | 机制 | 类比 |
|------|------|------|
| UART RTS/CTS | 额外 2 线，接收方快满时拉 RTS | 带外流控（外接红灯） |
| I2C 时钟拉伸 | 从机拉低 SCL | 带内流控（协议内置） |

> 日常使用通常关闭：`USART_HardwareFlowControl_None`

---

## 三、心得

- NVIC 的"数值越小越优先"是反直觉的，需要反复记忆
- IRQn 是索引、EXTI_Line 是位掩码——混淆这个会导致编译通过但行为异常
- EXTI 的 AFIO 绑定是容易被跳过的步骤：**先 GPIO_EXTILineConfig，再 EXTI_Init**
- 轮询+中断同时抢一个 LED 的教训非常深刻——这是嵌入式并发冲突的缩影
- ISR 应短小精悍，死等按键虽然能跑但不是最佳实践（后续用状态机改进）