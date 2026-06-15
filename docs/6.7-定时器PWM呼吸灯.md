# 6.7 定时器 — 时基单元 + PWM 呼吸灯

> **芯片**：STM32F103C8T6 | **环境**：VSCode + Keil + Keil Assistant  
> **主题**：时基单元、非阻塞延时、互补 PWM 输出、正弦呼吸灯

---

## 一、核心代码

### 1. TIM2 中断驱动毫秒计数器（替代 GetTick）

```c
volatile uint32_t currentTick = 0;  // volatile：中断和主循环都访问

void Init_TIM2(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseInitTypeDef tim = {0};
    tim.TIM_Prescaler         = 71;              // 72M / (71+1) = 1MHz = 1μs
    tim.TIM_Period            = 999;             // 1μs × 1000 = 1ms
    tim.TIM_CounterMode       = TIM_CounterMode_Up;
    tim.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &tim);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);   // 使能更新中断
    NVIC_Init(/* TIM2_IRQn */);
    TIM_Cmd(TIM2, ENABLE);
}

// ISR：每 1ms 触发一次
void TIM2_IRQHandler(void)
{
    if (TIM_GetFlagStatus(TIM2, TIM_IT_Update) == SET) {
        TIM_ClearFlag(TIM2, TIM_IT_Update);
        currentTick++;  // 先清标志，再操作
    }
}
```

### 2. 非阻塞延时（取代 Delay）

```c
void My_Delay_1(uint32_t ms)
{
    uint32_t expire = currentTick + ms;
    while (currentTick < expire);  // 忙等但不阻塞中断
}

// 使用状态机实现 1s 闪烁（完全不占 CPU）
void Process_mydelay(void)
{
    static uint32_t last = 0;
    static uint8_t  flag = 0;

    if (LED_MODE == LED_ON) {
        LED_SET();
        if (!flag) { last = currentTick; flag = 1; }
        if (currentTick - last >= 1000) { LED_MODE = LED_OFF; flag = 0; }
    } else {
        LED_RESET();
        if (!flag) { last = currentTick; flag = 1; }
        if (currentTick - last >= 1000) { LED_MODE = LED_ON; flag = 0; }
    }
}
```

### 3. TIM1 互补 PWM + 正弦呼吸灯

```c
void init_CCR1_Compare(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    // 时基：1ms 周期
    TIM_TimeBaseInitTypeDef tim = {0};
    tim.TIM_Prescaler = 71;   // 1MHz
    tim.TIM_Period    = 999;  // 1ms
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM1, &tim);

    TIM_Cmd(TIM1, ENABLE);
    TIM_CtrlPWMOutputs(TIM1, ENABLE);  // MOE — 高级定时器必须开！

    // 输出比较：CH1(PA8) + CH1N(PB13) 互补
    TIM_OCInitTypeDef oc = {0};
    oc.TIM_OCMode      = TIM_OCMode_PWM1;
    oc.TIM_OCPolarity  = TIM_OCPolarity_High;
    oc.TIM_OCNPolarity = TIM_OCPolarity_High;     // 互补极性与 CH1 相同 → 交替亮
    oc.TIM_OutputState  = TIM_OutputState_Enable;
    oc.TIM_OutputNState = TIM_OutputNState_Enable;
    oc.TIM_Pulse        = 0;                      // 初始占空比 0
    TIM_OC1Init(TIM1, &oc);
}

// 主循环：正弦动态占空比
void Process_Breathing_lamp(void)
{
    init_CCR1_Compare();
    Init_TIM2();

    while (1)
    {
        float t    = currentTick * 1.0e-3f;               // ms → s
        float duty = 0.5f * (sinf(2.0f * 3.1416f * t) + 1); // 0~1 正弦
        uint16_t ccr = (uint16_t)(duty * 1000.0f);        // CCR = 占空比 × 周期
        TIM_SetCompare1(TIM1, ccr);                       // 动态更新 CCR
    }
}
```

---

## 二、核心知识点

### 1. 时基单元四要素

| 寄存器 | 作用 | 公式 |
|--------|------|------|
| **PSC** | 预分频，降低输入时钟 | `f_tim = f_in / (PSC+1)` |
| **ARR** | 自动重装，决定周期 | `周期 = ARR + 1` |
| **CNT** | 计数器，对分频后脉冲计数 | 到 ARR 后归零 |
| **RCR** | 重复计数器 | `N 次溢出 = RCR + 1` |

> 例：72MHz → PSC=71 → 1MHz → ARR=999 → 1ms 中断

### 2. 影子寄存器（预装载）

PSC/ARR/RCR 都有影子寄存器。写入后等到**更新事件**才生效，防止 PWM 运行中改参数产生毛刺。ARR 预装载需手动开启 `ARPE`。

### 3. PWM 占空比公式

```
CCR = 占空比(0~1) × (ARR + 1)
```

PWM 模式 1：CNT < CCR → 有效电平；CNT ≥ CCR → 无效电平

### 4. 高级定时器 MOE

```c
TIM_CtrlPWMOutputs(TIM1, ENABLE);  // 必须调用，否则引脚无波形
```

TIM1/TIM8 是高级定时器，有刹车/死区功能，MOE（主输出使能）是总开关。

### 5. 互补输出

- CHx 和 CHxN 电平相反（经过反相器的话）
- 若极性设相同（`High` + `High`），则 CHxN 自动反相
- 死区时间由 `TIM_BDTRInitTypeDef` 配置

### 6. Cortex-M3 浮点优化

```c
// ❌ 双精度（Cortex-M3 无硬件 FPU，极慢）
float duty = 0.5 * (sin(2.0 * 3.1415926 * t) + 1);

// ✅ 单精度
float duty = 0.5f * (sinf(2.0f * 3.1416f * t) + 1);
```

`f` 后缀 + `sinf` 避免双精度软浮点开销。

### 7. Goto 使用边界

| ✅ 可用 | ❌ 禁止 |
|---------|---------|
| 向后跳到统一清理/错误处理 | 向前跳转 |
| 跳出多重循环 | 交叉逻辑 |
| Linux 内核常见模式 | 作为常规控制流 |

### 8. 系统错误处理状态机

```c
// 错误码 → 标志位 → 主循环降级处理
if (system_err_flag) {
    switch (System_State) {
        case SYSTEM_HSE_TIMEOUT:  /* 处理 */ break;
        case SYSTEM_PLL_TIMEOUT:  /* 处理 */ break;
        case SYSTEM_SELECT_ERR:   /* 切回 HSI */ break;
    }
}
```

> 避免 `while(1)` 死等，让系统有自愈能力

---

## 三、心得

- 定时器中断驱动的 `currentTick` 是裸机时间系统的标配——比 `GetTick` 更底层、更可控
- PWM 呼吸灯是感官上最"看得见"的成就：公式 → 寄存器 → 波形 → 灯光，闭环感极强
- `TIM_CtrlPWMOutputs` 是高级定时器的"隐藏开关"，忘开是经典坑
- 浮点数 `f` 后缀在 Cortex-M3 上不是偏好是必需