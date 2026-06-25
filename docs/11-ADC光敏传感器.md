# 6.11 ADC 光敏传感器 + 回调函数首次实战

> **芯片**：STM32F103C8T6 | **环境**：VSCode + Keil + Keil Assistant  
> **主题**：逐次逼近 ADC、光敏分压、按键消抖状态机封装、首次独立编写回调函数

---

## 一、核心代码

### 1. ADC 初始化（光敏传感器 PA0）

```c
void Init_photosensitive_sensor(void)
{
    // PA0 → 模拟输入
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitTypeDef g = {0};
    g.GPIO_Pin  = GPIO_Pin_0;
    g.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &g);

    // ADC 时钟 6 分频：72M/6 = 12MHz（≤14MHz）
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    ADC_InitTypeDef adc = {0};
    adc.ADC_Mode               = ADC_Mode_Independent;
    adc.ADC_ContinuousConvMode = DISABLE;           // 单次转换
    adc.ADC_ScanConvMode       = DISABLE;           // 不扫描
    adc.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None; // 软件触发
    adc.ADC_DataAlign          = ADC_DataAlign_Right;
    adc.ADC_NbrOfChannel       = 1;
    ADC_Init(ADC1, &adc);

    // 常规序列：通道 0，序号 1，采样 13.5 周期
    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_13Cycles5);
}
```

### 2. ADC 采样状态机（6 状态，非阻塞）

```c
typedef enum { PS_INIT, PS_SEND_PULSE, PS_WAIT_EOC,
               PS_PROCESS, PS_INTERVAL, PS_RESTART } ps_state_t;

void photosensitive_sensor_state_machine(void)
{
    switch (ps_state)
    {
    case PS_INIT:
        ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
        ADC_Cmd(ADC1, DISABLE);
        ps_state = PS_SEND_PULSE;
        break;

    case PS_SEND_PULSE:
        ADC_Cmd(ADC1, ENABLE);
        ADC_SoftwareStartConvCmd(ADC1, ENABLE);
        ps_time = currentTick;
        ps_state = PS_WAIT_EOC;
        break;

    case PS_WAIT_EOC:
        if (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == SET)
            ps_state = PS_PROCESS;
        else if (currentTick - ps_time > 30)  // 30ms 超时
            ps_state = PS_INTERVAL;
        break;

    case PS_PROCESS:
        uint16_t dr = ADC_GetConversionValue(ADC1);
        float voltage = dr * (3.3f / 4095);    // 12位 → 电压
        printf("ADC: %d, Voltage: %.3f V\r\n", dr, voltage);

        if (voltage > 1.5) LED_STATE = LED_OFF_1;  // 光线亮 → 关灯
        else               LED_STATE = LED_ON_1;   // 光线暗 → 开灯

        ps_time = currentTick;
        ps_state = PS_INTERVAL;
        break;

    case PS_INTERVAL:
        if (currentTick - ps_time > 1000)  // 1s 采集一次
            ps_state = PS_INIT;
        break;

    case PS_RESTART:
        if (currentTick - ps_time > 5000)  // 按键触发后 5s 重启
            ps_state = PS_INIT;
        break;
    }
}
```

### 3. 按键消抖状态机（通用封装 + 回调函数）

```c
typedef void (*key_callback_t)(void);

typedef enum { KEY_IDLE, PRESS_DEBOUNCE, PRESSED, RELEASE_DEBOUNCE } key_state_t;

typedef struct {
    key_state_t     state;
    uint32_t        tick;
    uint8_t         press_flag;
    key_callback_t  on_press;   // 回调函数指针
} key_debounce_t;

key_debounce_t key1;

void Key_Debounce_StateMachine(GPIO_TypeDef *GPIOx, uint16_t Pin, key_debounce_t *key)
{
    uint8_t level = GPIO_ReadInputDataBit(GPIOx, Pin);
    switch (key->state)
    {
    case KEY_IDLE:
        if (level == RESET) { key->tick = currentTick; key->state = PRESS_DEBOUNCE; }
        break;
    case PRESS_DEBOUNCE:
        if (level != RESET) key->state = KEY_IDLE;
        else if (currentTick - key->tick > DEBOUNCE_MS) {
            key->press_flag = 1;
            key->state = PRESSED;
            if (key->on_press) key->on_press();  // 回调！
        }
        break;
    case PRESSED:
        if (level != RESET) { key->tick = currentTick; key->state = RELEASE_DEBOUNCE; }
        break;
    case RELEASE_DEBOUNCE:
        if (level == RESET) key->state = PRESSED;
        else if (currentTick - key->tick > DEBOUNCE_MS) key->state = KEY_IDLE;
        break;
    }
}

// 回调函数：按键按下 → 5s 后重启 ADC 采集
void key_callback_restart(void)
{
    ps_time = currentTick;
    ps_state = PS_RESTART;
}

// 注册回调
void Key_Init(void)
{
    key1.state = KEY_IDLE;
    key1.on_press = key_callback_restart;  // 绑定回调
}
```

### 4. 主循环（三状态机并行）

```c
void Process_photosensitive_sensor(void)
{
    Init_photosensitive_sensor();
    Init_TIM2();
    Key_Init();
    Init_USART1();

    while (1)
    {
        photosensitive_sensor_state_machine();        // ADC 采集
        Key_Debounce_StateMachine(GPIOA, Pin_2, &key1); // 按键消抖
        LED_1_state_machine();                         // LED 控制
    }
}
```

---

## 二、核心知识点

### 1. ADC 逐次逼近原理

```
模拟电压 → 逐次比较 → 12位数字量 (0~4095)
LSB = Vref / 4096 ≈ 3.3V / 4096 ≈ 0.8mV
```

### 2. 转换时间计算

```
ADC 时钟 ≤ 14MHz → 72M / 6 = 12MHz
转换时间 = (采样周期 + 12.5) × ADC时钟周期
         = (13.5 + 12.5) × 1/12M ≈ 2.17μs
```

### 3. 光敏电阻分压

```
VCC ── 10kΩ ──┬── AO ── ADC
               │
              R₂（光敏）
               │
              GND

AO = 3.3V × R₂ / (10k + R₂)
光照强 → R₂小 → AO低（<1.5V → 开灯）
光照弱 → R₂大 → AO高（>1.5V → 关灯）
```

### 4. ADC 双序列架构

| 序列 | 通道数 | 结果寄存器 | 特点 |
|------|--------|-----------|------|
| **常规序列** | 1~16 | 共用 1 个 DR | 可 DMA |
| **注入序列** | 1~4 | 各通道独立 JDRx | 优先级高，可打断常规 |

### 5. 按键消抖状态机 — 通用封装

```
KEY_IDLE → PRESS_DEBOUNCE → PRESSED → RELEASE_DEBOUNCE → KEY_IDLE
  ↑         (20ms确认)       (回调!)      (20ms确认)         ↑
  └─────────────────────────────────────────────────────────┘
```

### 6. 回调函数 — 控制反转

```c
// 传统方式：按键模块直接调业务函数（紧耦合）
if (key_pressed) do_restart();

// 回调方式：注册函数指针，按键模块不知道业务逻辑（松耦合）
key.on_press = key_callback_restart;  // 注册
// 按键模块内部：if (on_press) on_press();  // 调用
```

> 这是你第一次独立实现回调——从"过程调用"升级为"事件注册"

---

## 三、心得

- ADC 的采样时间与信号源内阻有关，光敏电阻分压需配 13.5 周期——不是随便选的
- 按键消抖用结构体 `key_debounce_t` 封装后直接可移植到任何项目，这就是模块化的力量
- 回调函数让按键模块和业务逻辑完全解耦——你按下的是按钮，触发的是"事件"，这是裸机架构向 RTOS 思维过渡的关键一步
- 三个状态机在主循环中并行跑，CPU 几乎零等待——裸机前后台架构的完美示范