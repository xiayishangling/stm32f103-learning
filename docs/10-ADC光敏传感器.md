# 6.11 ADC 鍏夋晱浼犳劅鍣?+ 鍥炶皟鍑芥暟棣栨瀹炴垬

> **鑺墖**锛歋TM32F103C8T6 | **鐜**锛歏SCode + Keil + Keil Assistant  
> **涓婚**锛氶€愭閫艰繎 ADC銆佸厜鏁忓垎鍘嬨€佹寜閿秷鎶栫姸鎬佹満灏佽銆侀娆＄嫭绔嬬紪鍐欏洖璋冨嚱鏁?
---

## 涓€銆佹牳蹇冧唬鐮?
### 1. ADC 鍒濆鍖栵紙鍏夋晱浼犳劅鍣?PA0锛?
```c
void Init_photosensitive_sensor(void)
{
    // PA0 鈫?妯℃嫙杈撳叆
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitTypeDef g = {0};
    g.GPIO_Pin  = GPIO_Pin_0;
    g.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &g);

    // ADC 鏃堕挓 6 鍒嗛锛?2M/6 = 12MHz锛堚墹14MHz锛?    RCC_ADCCLKConfig(RCC_PCLK2_Div6);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    ADC_InitTypeDef adc = {0};
    adc.ADC_Mode               = ADC_Mode_Independent;
    adc.ADC_ContinuousConvMode = DISABLE;           // 鍗曟杞崲
    adc.ADC_ScanConvMode       = DISABLE;           // 涓嶆壂鎻?    adc.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None; // 杞欢瑙﹀彂
    adc.ADC_DataAlign          = ADC_DataAlign_Right;
    adc.ADC_NbrOfChannel       = 1;
    ADC_Init(ADC1, &adc);

    // 甯歌搴忓垪锛氶€氶亾 0锛屽簭鍙?1锛岄噰鏍?13.5 鍛ㄦ湡
    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_13Cycles5);
}
```

### 2. ADC 閲囨牱鐘舵€佹満锛? 鐘舵€侊紝闈為樆濉烇級

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
        else if (currentTick - ps_time > 30)  // 30ms 瓒呮椂
            ps_state = PS_INTERVAL;
        break;

    case PS_PROCESS:
        uint16_t dr = ADC_GetConversionValue(ADC1);
        float voltage = dr * (3.3f / 4095);    // 12浣?鈫?鐢靛帇
        printf("ADC: %d, Voltage: %.3f V\r\n", dr, voltage);

        if (voltage > 1.5) LED_STATE = LED_OFF_1;  // 鍏夌嚎浜?鈫?鍏崇伅
        else               LED_STATE = LED_ON_1;   // 鍏夌嚎鏆?鈫?寮€鐏?
        ps_time = currentTick;
        ps_state = PS_INTERVAL;
        break;

    case PS_INTERVAL:
        if (currentTick - ps_time > 1000)  // 1s 閲囬泦涓€娆?            ps_state = PS_INIT;
        break;

    case PS_RESTART:
        if (currentTick - ps_time > 5000)  // 鎸夐敭瑙﹀彂鍚?5s 閲嶅惎
            ps_state = PS_INIT;
        break;
    }
}
```

### 3. 鎸夐敭娑堟姈鐘舵€佹満锛堥€氱敤灏佽 + 鍥炶皟鍑芥暟锛?
```c
typedef void (*key_callback_t)(void);

typedef enum { KEY_IDLE, PRESS_DEBOUNCE, PRESSED, RELEASE_DEBOUNCE } key_state_t;

typedef struct {
    key_state_t     state;
    uint32_t        tick;
    uint8_t         press_flag;
    key_callback_t  on_press;   // 鍥炶皟鍑芥暟鎸囬拡
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
            if (key->on_press) key->on_press();  // 鍥炶皟锛?        }
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

// 鍥炶皟鍑芥暟锛氭寜閿寜涓?鈫?5s 鍚庨噸鍚?ADC 閲囬泦
void key_callback_restart(void)
{
    ps_time = currentTick;
    ps_state = PS_RESTART;
}

// 娉ㄥ唽鍥炶皟
void Key_Init(void)
{
    key1.state = KEY_IDLE;
    key1.on_press = key_callback_restart;  // 缁戝畾鍥炶皟
}
```

### 4. 涓诲惊鐜紙涓夌姸鎬佹満骞惰锛?
```c
void Process_photosensitive_sensor(void)
{
    Init_photosensitive_sensor();
    Init_TIM2();
    Key_Init();
    Init_USART1();

    while (1)
    {
        photosensitive_sensor_state_machine();        // ADC 閲囬泦
        Key_Debounce_StateMachine(GPIOA, Pin_2, &key1); // 鎸夐敭娑堟姈
        LED_1_state_machine();                         // LED 鎺у埗
    }
}
```

---

## 浜屻€佹牳蹇冪煡璇嗙偣

### 1. ADC 閫愭閫艰繎鍘熺悊

```
妯℃嫙鐢靛帇 鈫?閫愭姣旇緝 鈫?12浣嶆暟瀛楅噺 (0~4095)
LSB = Vref / 4096 鈮?3.3V / 4096 鈮?0.8mV
```

### 2. 杞崲鏃堕棿璁＄畻

```
ADC 鏃堕挓 鈮?14MHz 鈫?72M / 6 = 12MHz
杞崲鏃堕棿 = (閲囨牱鍛ㄦ湡 + 12.5) 脳 ADC鏃堕挓鍛ㄦ湡
         = (13.5 + 12.5) 脳 1/12M 鈮?2.17渭s
```

### 3. 鍏夋晱鐢甸樆鍒嗗帇

```
VCC 鈹€鈹€ 10k惟 鈹€鈹€鈹攢鈹€ AO 鈹€鈹€ ADC
               鈹?              R鈧傦紙鍏夋晱锛?               鈹?              GND

AO = 3.3V 脳 R鈧?/ (10k + R鈧?
鍏夌収寮?鈫?R鈧傚皬 鈫?AO浣庯紙<1.5V 鈫?寮€鐏級
鍏夌収寮?鈫?R鈧傚ぇ 鈫?AO楂橈紙>1.5V 鈫?鍏崇伅锛?```

### 4. ADC 鍙屽簭鍒楁灦鏋?
| 搴忓垪 | 閫氶亾鏁?| 缁撴灉瀵勫瓨鍣?| 鐗圭偣 |
|------|--------|-----------|------|
| **甯歌搴忓垪** | 1~16 | 鍏辩敤 1 涓?DR | 鍙?DMA |
| **娉ㄥ叆搴忓垪** | 1~4 | 鍚勯€氶亾鐙珛 JDRx | 浼樺厛绾ч珮锛屽彲鎵撴柇甯歌 |

### 5. 鎸夐敭娑堟姈鐘舵€佹満 鈥?閫氱敤灏佽

```
KEY_IDLE 鈫?PRESS_DEBOUNCE 鈫?PRESSED 鈫?RELEASE_DEBOUNCE 鈫?KEY_IDLE
  鈫?        (20ms纭)       (鍥炶皟!)      (20ms纭)         鈫?  鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?```

### 6. 鍥炶皟鍑芥暟 鈥?鎺у埗鍙嶈浆

```c
// 浼犵粺鏂瑰紡锛氭寜閿ā鍧楃洿鎺ヨ皟涓氬姟鍑芥暟锛堢揣鑰﹀悎锛?if (key_pressed) do_restart();

// 鍥炶皟鏂瑰紡锛氭敞鍐屽嚱鏁版寚閽堬紝鎸夐敭妯″潡涓嶇煡閬撲笟鍔￠€昏緫锛堟澗鑰﹀悎锛?key.on_press = key_callback_restart;  // 娉ㄥ唽
// 鎸夐敭妯″潡鍐呴儴锛歩f (on_press) on_press();  // 璋冪敤
```

> 杩欐槸浣犵涓€娆＄嫭绔嬪疄鐜板洖璋冣€斺€斾粠"杩囩▼璋冪敤"鍗囩骇涓?浜嬩欢娉ㄥ唽"

---

## 涓夈€佸績寰?
- ADC 鐨勯噰鏍锋椂闂翠笌淇″彿婧愬唴闃绘湁鍏筹紝鍏夋晱鐢甸樆鍒嗗帇闇€閰?13.5 鍛ㄦ湡鈥斺€斾笉鏄殢渚块€夌殑
- 鎸夐敭娑堟姈鐢ㄧ粨鏋勪綋 `key_debounce_t` 灏佽鍚庣洿鎺ュ彲绉绘鍒颁换浣曢」鐩紝杩欏氨鏄ā鍧楀寲鐨勫姏閲?- 鍥炶皟鍑芥暟璁╂寜閿ā鍧楀拰涓氬姟閫昏緫瀹屽叏瑙ｈ€︹€斺€斾綘鎸変笅鐨勬槸鎸夐挳锛岃Е鍙戠殑鏄?浜嬩欢"锛岃繖鏄８鏈烘灦鏋勫悜 RTOS 鎬濈淮杩囨浮鐨勫叧閿竴姝?- 涓変釜鐘舵€佹満鍦ㄤ富寰幆涓苟琛岃窇锛孋PU 鍑犱箮闆剁瓑寰呪€斺€旇８鏈哄墠鍚庡彴鏋舵瀯鐨勫畬缇庣ず鑼
