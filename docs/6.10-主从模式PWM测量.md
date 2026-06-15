# 6.10 主从模式 + PWM 信号测量

> **芯片**：STM32F103C8T6 | **环境**：VSCode + Keil + Keil Assistant  
> **主题**：从模式复位、主模式 TRGO、双定时器协同、PWM 周期/占空比测量

---

## 一、核心代码

### 1. 产生被测 PWM（TIM3_CH1 → PA6）

```c
void Init_OC_generatePWM(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    // 时基：1ms 周期
    TIM_TimeBaseInitTypeDef tim = {0};
    tim.TIM_Prescaler = 71;     // 1MHz
    tim.TIM_Period    = 999;    // 1ms
    TIM_TimeBaseInit(TIM3, &tim);
    TIM_ARRPreloadConfig(TIM3, ENABLE);  // ARR 预装载，防毛刺

    // PWM 输出
    TIM_OCInitTypeDef oc = {0};
    oc.TIM_OCMode      = TIM_OCMode_PWM1;
    oc.TIM_OCPolarity  = TIM_OCPolarity_High;
    oc.TIM_OutputState = TIM_OutputState_Enable;
    oc.TIM_Pulse       = 200;   // 20% 占空比
    TIM_OC1Init(TIM3, &oc);

    TIM_CCPreloadControl(TIM3, ENABLE);  // CCR 预装载
    TIM_CtrlPWMOutputs(TIM3, ENABLE);
    TIM_Cmd(TIM3, ENABLE);
}
```

### 2. 测量 PWM（TIM1 从模式复位 + 双通道捕获）

```c
void Init_IC_measurePWM(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    // 时基：1μs 分辨率
    TIM_TimeBaseInitTypeDef tim = {0};
    tim.TIM_Prescaler = 71;
    tim.TIM_Period    = 65535;
    TIM_TimeBaseInit(TIM1, &tim);

    // CH1：直接上升沿捕获（占空比 = CCR2 - 0）
    TIM_ICInitTypeDef ic = {0};
    ic.TIM_Channel    = TIM_Channel_1;
    ic.TIM_ICPolarity = TIM_ICPolarity_Rising;
    ic.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICInit(TIM1, &ic);

    // CH2：间接下降沿捕获
    ic.TIM_Channel    = TIM_Channel_2;
    ic.TIM_ICPolarity = TIM_ICPolarity_Falling;
    ic.TIM_ICSelection = TIM_ICSelection_IndirectTI;
    TIM_ICInit(TIM1, &ic);

    // 【关键】从模式：TRGI 上升沿自动清零 CNT
    TIM_SelectInputTrigger(TIM1, TIM_TS_TI1FP1);      // 触发源 = CH1 滤波后
    TIM_SelectSlaveMode(TIM1, TIM_SlaveMode_Reset);    // 复位模式

    TIM_Cmd(TIM1, ENABLE);
}
```

### 3. PWM 测量状态机（轮询标志位，无需 CC 中断！）

```c
void Measure_PWM_state_machine(void)
{
    switch (measure_state)
    {
    case MS_INIT:
        TIM_SetCompare1(TIM3, 200);                    // 设占空比 20%
        TIM_ClearFlag(TIM1, TIM_FLAG_Trigger | TIM_FLAG_CC1 | TIM_FLAG_CC2);
        ms_time = currentTick;
        measure_state = WAIT_TRIGGER;
        break;

    case WAIT_TRIGGER:
        if (TIM_GetFlagStatus(TIM1, TIM_FLAG_Trigger) == SET)
            measure_state = MS_PROCESS;                 // 触发到了
        else if (currentTick - ms_time > 20)
            measure_state = INTER_VAL;                  // 超时
        break;

    case MS_PROCESS:
        if (TIM_GetFlagStatus(TIM1, TIM_FLAG_CC2) == SET)  // 等下降沿
        {
            TIM_ClearFlag(TIM1, TIM_FLAG_CC2);
            ms_cc1 = TIM_GetCapture1(TIM1);             // 周期 (μs)
            ms_cc2 = TIM_GetCapture2(TIM1);             // 高电平宽度 (μs)

            float period = ms_cc1 * 1.0e-6f * 1.0e3f;   // μs → ms
            float duty   = (float)ms_cc2 / ms_cc1 * 100.0f;  // %
            printf("Period=%.3fms  Duty=%.2f%%\r\n", period, duty);

            ms_time = currentTick;
            measure_state = INTER_VAL;
        }
        break;

    case INTER_VAL:
        if (currentTick - ms_time > 1000)                // 1s 测一次
            measure_state = MS_INIT;
        break;
    }
}
```

---

## 二、核心知识点

### 1. 从模式 vs 主模式

| 方向 | 模式 | 作用 |
|------|------|------|
| **从模式** | 接受外部触发（TRGI）控制计数 | 复位、门控、触发、外部时钟 |
| **主模式** | 通过 TRGO 输出内部事件 | 更新、使能、比较脉冲、OCxREF |

> 6.10 用的从模式：`TIM_SlaveMode_Reset` — TRGI 上升沿自动清零 CNT

### 2. 为什么复位模式省掉了回绕计算

```
从模式复位前：
  CCR1 = 500, CCR2 = 300  →  脉宽 = 300, 但周期 = ?（需计算 65536-500+...）

从模式复位后：
  每个 PWM 上升沿 → CNT 自动归零
  CCR1 = 周期(1000), CCR2 = 脉宽(200)
  占空比 = CCR2 / CCR1，无需任何回绕处理！
```

### 3. 信号链完整路径

```
PA6(TIM3_CH1 PWM) ──物理连线──→ PA8(TIM1_CH1)
                                    ↓
                          输入滤波 → 边沿检测(上升沿)
                                    ↓
                    ┌→ CH1 直接捕获 → CCR1（周期）
                    │
                    └→ TRGI(从模式复位 CNT) + CH2 间接捕获 → CCR2（脉宽）
```

### 4. 为何不需要 CC 中断？

- 从模式自动复位 CNT，主循环只需轮询 `TIM_FLAG_Trigger` + `TIM_FLAG_CC2`
- 状态机在 `ms_init` 中 `TIM_ClearFlag` 清所有残余标志
- 主循环无任何阻塞等待，CPU 占用极低

### 5. ARR 和 CCR 预装载

```c
TIM_ARRPreloadConfig(TIM3, ENABLE);   // ARR 影子寄存器
TIM_CCPreloadControl(TIM3, ENABLE);   // CCR 影子寄存器
```

- 运行时改 ARR/CCR，等到更新事件才生效
- 避免 PWM 运行中改参数产生毛刺

### 6. 从模式 8 种速查

| 模式 | 作用 |
|------|------|
| **Reset** | TRGI 上升沿清零 CNT |
| Gated | TRGI 高时计数 |
| Trigger | TRGI 上升沿启动一次计数 |
| External Clock 1 | TRGI 每个上升沿计数一次 |

---

## 三、代码评价

- **亮点**：用从模式复位替代软件回绕计算，深刻理解了"用硬件特性省软件干预"
- **架构**：双定时器 + 双状态机 + 标志位轮询，CPU 几乎零等待
- **防御性**：`ms_init` 中清除所有可能残留的标志位，`ms_time=0` 防野值
- **可优化**：`sin_flicker()` 在主循环浮点运算略重，可用查表法加速

---

## 四、心得

- 从模式复位是输入捕获的"作弊码"——硬件自动清零 CNT，回绕计算成为历史
- `TIM_SelectInputTrigger` vs `TIM_SelectOutputTrigger` 一字之差，方向完全不同
- 轮询标志位 + 状态机 可以实现无中断的精确测量——前提是主循环够快