# 6.14 ADC 定时器触发注入序列 + 扫描模式

> **芯片**：STM32F103C8T6 | **环境**：VSCode + Keil + Keil Assistant  
> **主题**：TIM1_TRGO 触发 ADC、注入序列、扫描模式、多路采集

---

## 一、核心代码

### 1. 定时器触发注入序列配置

```c
void Init_TIM_Injected_Sequence(void)
{
    // ----- TIM1: 每 1ms 产生更新事件作 TRGO -----
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    TIM_TimeBaseInitTypeDef tim = {0};
    tim.TIM_Prescaler = 71;
    tim.TIM_Period    = 999;    // 1ms
    TIM_TimeBaseInit(TIM1, &tim);
    TIM_SelectOutputTrigger(TIM1, TIM_TRGOSource_Update);  // 主模式：输出更新事件
    TIM_Cmd(TIM1, ENABLE);

    // ----- ADC: 注入序列，TIM1_TRGO 触发 -----
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    ADC_InitTypeDef adc = {0};
    adc.ADC_Mode         = ADC_Mode_Independent;
    adc.ADC_ScanConvMode = DISABLE;
    adc.ADC_ContinuousConvMode = DISABLE;
    adc.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;
    ADC_Init(ADC1, &adc);

    // 注入序列：1 个通道，TIM1_TRGO 触发
    ADC_InjectedSequencerLengthConfig(ADC1, 1);
    ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_T1_TRGO);
    ADC_ExternalTrigInjectedConvCmd(ADC1, ENABLE);
    ADC_InjectedChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_13Cycles5);

    // 校准
    ADC_Cmd(ADC1, ENABLE);
    ADC_ResetCalibration(ADC1);  while (ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);  while (ADC_GetCalibrationStatus(ADC1));
    ADC_Cmd(ADC1, DISABLE);      // 留给状态机控制
}
```

### 2. JEOC 中断 + 注入序列状态机

```c
// 中断：只置标志位
void ADC1_2_IRQHandler(void)
{
    if (ADC_GetFlagStatus(ADC1, ADC_FLAG_JEOC) == SET) {
        ADC_ClearFlag(ADC1, ADC_FLAG_JEOC);
        PS_STATE = PS_OK;   // 通知主循环
    }
}

// 状态机：触发 → 等 JEOC → 处理 → 间隔
void IS_state_machine(void)
{
    switch (ps_state) {
    case PS_INIT:
        ADC_ClearFlag(ADC1, ADC_FLAG_JEOC);
        ADC_Cmd(ADC1, DISABLE);
        ps_state = PS_SEND_PULSE;
        break;

    case PS_SEND_PULSE:
        ADC_Cmd(ADC1, ENABLE);   // 开 ADC，TIM1_TRGO 自动每 1ms 触发一次
        ps_state = PS_WAIT_EOC;
        break;

    case PS_WAIT_EOC:
        if (PS_STATE == PS_OK) {         // JEOC 中断已置标志
            PS_STATE = PS_NO;
            ps_state = PS_PROCESS;
        } else if (currentTick - ps_time > 20)  // 20ms 超时
            ps_state = PS_INTERVAL;
        break;

    case PS_PROCESS:
        uint16_t jdr = ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_1);
        ADC_Cmd(ADC1, DISABLE);
        float v = jdr * (3.3f / 4095);
        printf("%.3f V\r\n", v);
        v > 1.5 ? (LED_STATE = LED_ON) : (LED_STATE = LED_OFF);
        ps_state = PS_INTERVAL;
        break;

    case PS_INTERVAL:
        ADC_Cmd(ADC1, ENABLE);           // 重新开，等下一次 JEOC
        ps_state = PS_WAIT_EOC;
        break;
    }
}
```

### 3. 扫描模式（双通道自动采集）

```c
void Init_ADC_scan_patterns(void)
{
    // ----- 双通道配置 -----
    ADC_InitTypeDef adc = {0};
    adc.ADC_ScanConvMode = ENABLE;    // 开启扫描
    adc.ADC_NbrOfChannel = 2;         // 2 个通道
    ADC_Init(ADC1, &adc);

    ADC_InjectedSequencerLengthConfig(ADC1, 2);
    ADC_InjectedChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_13Cycles5);
    ADC_InjectedChannelConfig(ADC1, ADC_Channel_1, 2, ADC_SampleTime_13Cycles5);
    // 触发一次 → 硬件自动依次转换 CH0 → CH1
}

void scan_state_machine(void)
{
    // ... 同注入序列框架 ...
    case PS_PROCESS:
        uint16_t jdr1 = ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_1);
        uint16_t jdr2 = ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_2);
        float v1 = jdr1 * (3.3f / 4095);
        float v2 = jdr2 * (3.3f / 4095);

        // 双路组合逻辑控制 LED
        if (v1 > 1.5 && v2 > 1.5)       LED_STATE = LED_ON;       // 常亮
        else if (v1 < 1.5 && v2 < 1.5)  LED_STATE = LED_FLICKER;  // 闪烁
        else                             LED_STATE = LED_OFF;      // 熄灭
        break;
}
```

### 4. LED 闪烁子状态机

```c
void LED_1_state_machine(void)
{
    static uint32_t led_tick;

    switch (LED_STATE) {
    case LED_ON:      LED_SET();   break;
    case LED_OFF:     LED_RESET(); break;
    case LED_FLICKER:
        if (currentTick - led_tick > 500) {
            led_tick = currentTick;
            if (ledf_state == LED_FLICKER_ON) {
                LED_RESET(); ledf_state = LED_FLICKER_OFF;
            } else {
                LED_SET();   ledf_state = LED_FLICKER_ON;
            }
        }
        break;
    }
}
```

---

## 二、核心知识点

### 1. Input Trigger vs Output Trigger

| 函数 | 方向 | 寄存器 | 口诀 |
|------|------|--------|------|
| `TIM_SelectInputTrigger` | 外部→本定时器 | SMCR | **别人触发我** |
| `TIM_SelectOutputTrigger` | 本定时器→外部 | CR2 | **我触发别人** |

### 2. 定时器触发 ADC 完整链路

```
TIM1 每 1ms 更新事件 → TRGO 输出
    ↓
ADC1 注入序列外部触发源 = TIM1_TRGO
    ↓
TRGO 脉冲 → ADC 自动转换注入通道 → JEOC 中断
    ↓
ISR 置标志位 → 主循环状态机处理
```

> CPU 完全不参与触发，只取结果——效率远超软件轮询

### 3. 注入序列 vs 常规序列

| 特性 | 常规序列 | 注入序列 |
|------|----------|----------|
| 通道数 | 1~16 | 1~4 |
| 结果寄存器 | 共用 1 个 DR | 各通道独立 JDR1~4 |
| 优先级 | 普通 | **可抢占常规序列** |
| DMA | 支持 | 不支持 |
| 典型场景 | 周期性巡检 | 精确时刻采样（如相角触发） |

### 4. 扫描模式机制

```
触发一次 → CH0 采样 → 存 JDR1 → CH1 采样 → 存 JDR2 → JEOC 中断
```

无需每个通道独立触发，硬件自动按序完成。

### 5. ADC 校准流程

```c
ADC_Cmd(ADC1, ENABLE);
ADC_ResetCalibration(ADC1);   // 复位校准
while (ADC_GetResetCalibrationStatus(ADC1));
ADC_StartCalibration(ADC1);   // 启动校准
while (ADC_GetCalibrationStatus(ADC1));
```

> 校准应在 ADC 使能后进行，保证 12 位精度

### 6. 统一状态机框架

```
ps_init → ps_send_pulse → ps_wait_conv_ok → ps_process → ps_interval
   ↑        (使能ADC)      (等JEOC/超时)     (取结果)     (循环/重启)
   └────────────────── ps_restart (按键5s后) ──────────────┘
```

单通道、注入序列、扫描模式共用此框架，仅 `ps_process` 中读取逻辑不同。

---

## 三、心得

- `TIM_SelectOutputTrigger` + ADC 外部触发 = 嵌入式"自动驾驶"——CPU 零参与，定时精度远超软件方案
- 扫描模式让多路采集从"写 N 个状态机"变成"配 N 个通道"，硬件替你做了并行
- 从 6.1 的点亮 LED 到 6.14 的双通道定时器触发 ADC 扫描——14 天，你的状态机框架已成熟到"填空即用"的程度