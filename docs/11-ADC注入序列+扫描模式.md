# 6.14 ADC 瀹氭椂鍣ㄨЕ鍙戞敞鍏ュ簭鍒?+ 鎵弿妯″紡

> **鑺墖**锛歋TM32F103C8T6 | **鐜**锛歏SCode + Keil + Keil Assistant  
> **涓婚**锛歍IM1_TRGO 瑙﹀彂 ADC銆佹敞鍏ュ簭鍒椼€佹壂鎻忔ā寮忋€佸璺噰闆?
---

## 涓€銆佹牳蹇冧唬鐮?
### 1. 瀹氭椂鍣ㄨЕ鍙戞敞鍏ュ簭鍒楅厤缃?
```c
void Init_TIM_Injected_Sequence(void)
{
    // ----- TIM1: 姣?1ms 浜х敓鏇存柊浜嬩欢浣?TRGO -----
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    TIM_TimeBaseInitTypeDef tim = {0};
    tim.TIM_Prescaler = 71;
    tim.TIM_Period    = 999;    // 1ms
    TIM_TimeBaseInit(TIM1, &tim);
    TIM_SelectOutputTrigger(TIM1, TIM_TRGOSource_Update);  // 涓绘ā寮忥細杈撳嚭鏇存柊浜嬩欢
    TIM_Cmd(TIM1, ENABLE);

    // ----- ADC: 娉ㄥ叆搴忓垪锛孴IM1_TRGO 瑙﹀彂 -----
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    ADC_InitTypeDef adc = {0};
    adc.ADC_Mode         = ADC_Mode_Independent;
    adc.ADC_ScanConvMode = DISABLE;
    adc.ADC_ContinuousConvMode = DISABLE;
    adc.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;
    ADC_Init(ADC1, &adc);

    // 娉ㄥ叆搴忓垪锛? 涓€氶亾锛孴IM1_TRGO 瑙﹀彂
    ADC_InjectedSequencerLengthConfig(ADC1, 1);
    ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_T1_TRGO);
    ADC_ExternalTrigInjectedConvCmd(ADC1, ENABLE);
    ADC_InjectedChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_13Cycles5);

    // 鏍″噯
    ADC_Cmd(ADC1, ENABLE);
    ADC_ResetCalibration(ADC1);  while (ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);  while (ADC_GetCalibrationStatus(ADC1));
    ADC_Cmd(ADC1, DISABLE);      // 鐣欑粰鐘舵€佹満鎺у埗
}
```

### 2. JEOC 涓柇 + 娉ㄥ叆搴忓垪鐘舵€佹満

```c
// 涓柇锛氬彧缃爣蹇椾綅
void ADC1_2_IRQHandler(void)
{
    if (ADC_GetFlagStatus(ADC1, ADC_FLAG_JEOC) == SET) {
        ADC_ClearFlag(ADC1, ADC_FLAG_JEOC);
        PS_STATE = PS_OK;   // 閫氱煡涓诲惊鐜?    }
}

// 鐘舵€佹満锛氳Е鍙?鈫?绛?JEOC 鈫?澶勭悊 鈫?闂撮殧
void IS_state_machine(void)
{
    switch (ps_state) {
    case PS_INIT:
        ADC_ClearFlag(ADC1, ADC_FLAG_JEOC);
        ADC_Cmd(ADC1, DISABLE);
        ps_state = PS_SEND_PULSE;
        break;

    case PS_SEND_PULSE:
        ADC_Cmd(ADC1, ENABLE);   // 寮€ ADC锛孴IM1_TRGO 鑷姩姣?1ms 瑙﹀彂涓€娆?        ps_state = PS_WAIT_EOC;
        break;

    case PS_WAIT_EOC:
        if (PS_STATE == PS_OK) {         // JEOC 涓柇宸茬疆鏍囧織
            PS_STATE = PS_NO;
            ps_state = PS_PROCESS;
        } else if (currentTick - ps_time > 20)  // 20ms 瓒呮椂
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
        ADC_Cmd(ADC1, ENABLE);           // 閲嶆柊寮€锛岀瓑涓嬩竴娆?JEOC
        ps_state = PS_WAIT_EOC;
        break;
    }
}
```

### 3. 鎵弿妯″紡锛堝弻閫氶亾鑷姩閲囬泦锛?
```c
void Init_ADC_scan_patterns(void)
{
    // ----- 鍙岄€氶亾閰嶇疆 -----
    ADC_InitTypeDef adc = {0};
    adc.ADC_ScanConvMode = ENABLE;    // 寮€鍚壂鎻?    adc.ADC_NbrOfChannel = 2;         // 2 涓€氶亾
    ADC_Init(ADC1, &adc);

    ADC_InjectedSequencerLengthConfig(ADC1, 2);
    ADC_InjectedChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_13Cycles5);
    ADC_InjectedChannelConfig(ADC1, ADC_Channel_1, 2, ADC_SampleTime_13Cycles5);
    // 瑙﹀彂涓€娆?鈫?纭欢鑷姩渚濇杞崲 CH0 鈫?CH1
}

void scan_state_machine(void)
{
    // ... 鍚屾敞鍏ュ簭鍒楁鏋?...
    case PS_PROCESS:
        uint16_t jdr1 = ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_1);
        uint16_t jdr2 = ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_2);
        float v1 = jdr1 * (3.3f / 4095);
        float v2 = jdr2 * (3.3f / 4095);

        // 鍙岃矾缁勫悎閫昏緫鎺у埗 LED
        if (v1 > 1.5 && v2 > 1.5)       LED_STATE = LED_ON;       // 甯镐寒
        else if (v1 < 1.5 && v2 < 1.5)  LED_STATE = LED_FLICKER;  // 闂儊
        else                             LED_STATE = LED_OFF;      // 鐔勭伃
        break;
}
```

### 4. LED 闂儊瀛愮姸鎬佹満

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

## 浜屻€佹牳蹇冪煡璇嗙偣

### 1. Input Trigger vs Output Trigger

| 鍑芥暟 | 鏂瑰悜 | 瀵勫瓨鍣?| 鍙ｈ瘈 |
|------|------|--------|------|
| `TIM_SelectInputTrigger` | 澶栭儴鈫掓湰瀹氭椂鍣?| SMCR | **鍒汉瑙﹀彂鎴?* |
| `TIM_SelectOutputTrigger` | 鏈畾鏃跺櫒鈫掑閮?| CR2 | **鎴戣Е鍙戝埆浜?* |

### 2. 瀹氭椂鍣ㄨЕ鍙?ADC 瀹屾暣閾捐矾

```
TIM1 姣?1ms 鏇存柊浜嬩欢 鈫?TRGO 杈撳嚭
    鈫?ADC1 娉ㄥ叆搴忓垪澶栭儴瑙﹀彂婧?= TIM1_TRGO
    鈫?TRGO 鑴夊啿 鈫?ADC 鑷姩杞崲娉ㄥ叆閫氶亾 鈫?JEOC 涓柇
    鈫?ISR 缃爣蹇椾綅 鈫?涓诲惊鐜姸鎬佹満澶勭悊
```

> CPU 瀹屽叏涓嶅弬涓庤Е鍙戯紝鍙彇缁撴灉鈥斺€旀晥鐜囪繙瓒呰蒋浠惰疆璇?
### 3. 娉ㄥ叆搴忓垪 vs 甯歌搴忓垪

| 鐗规€?| 甯歌搴忓垪 | 娉ㄥ叆搴忓垪 |
|------|----------|----------|
| 閫氶亾鏁?| 1~16 | 1~4 |
| 缁撴灉瀵勫瓨鍣?| 鍏辩敤 1 涓?DR | 鍚勯€氶亾鐙珛 JDR1~4 |
| 浼樺厛绾?| 鏅€?| **鍙姠鍗犲父瑙勫簭鍒?* |
| DMA | 鏀寔 | 涓嶆敮鎸?|
| 鍏稿瀷鍦烘櫙 | 鍛ㄦ湡鎬у贰妫€ | 绮剧‘鏃跺埢閲囨牱锛堝鐩歌瑙﹀彂锛?|

### 4. 鎵弿妯″紡鏈哄埗

```
瑙﹀彂涓€娆?鈫?CH0 閲囨牱 鈫?瀛?JDR1 鈫?CH1 閲囨牱 鈫?瀛?JDR2 鈫?JEOC 涓柇
```

鏃犻渶姣忎釜閫氶亾鐙珛瑙﹀彂锛岀‖浠惰嚜鍔ㄦ寜搴忓畬鎴愩€?
### 5. ADC 鏍″噯娴佺▼

```c
ADC_Cmd(ADC1, ENABLE);
ADC_ResetCalibration(ADC1);   // 澶嶄綅鏍″噯
while (ADC_GetResetCalibrationStatus(ADC1));
ADC_StartCalibration(ADC1);   // 鍚姩鏍″噯
while (ADC_GetCalibrationStatus(ADC1));
```

> 鏍″噯搴斿湪 ADC 浣胯兘鍚庤繘琛岋紝淇濊瘉 12 浣嶇簿搴?
### 6. 缁熶竴鐘舵€佹満妗嗘灦

```
ps_init 鈫?ps_send_pulse 鈫?ps_wait_conv_ok 鈫?ps_process 鈫?ps_interval
   鈫?       (浣胯兘ADC)      (绛塉EOC/瓒呮椂)     (鍙栫粨鏋?     (寰幆/閲嶅惎)
   鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€ ps_restart (鎸夐敭5s鍚? 鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?```

鍗曢€氶亾銆佹敞鍏ュ簭鍒椼€佹壂鎻忔ā寮忓叡鐢ㄦ妗嗘灦锛屼粎 `ps_process` 涓鍙栭€昏緫涓嶅悓銆?
---

## 涓夈€佸績寰?
- `TIM_SelectOutputTrigger` + ADC 澶栭儴瑙﹀彂 = 宓屽叆寮?鑷姩椹鹃┒"鈥斺€擟PU 闆跺弬涓庯紝瀹氭椂绮惧害杩滆秴杞欢鏂规
- 鎵弿妯″紡璁╁璺噰闆嗕粠"鍐?N 涓姸鎬佹満"鍙樻垚"閰?N 涓€氶亾"锛岀‖浠舵浛浣犲仛浜嗗苟琛?- 浠?6.1 鐨勭偣浜?LED 鍒?6.14 鐨勫弻閫氶亾瀹氭椂鍣ㄨЕ鍙?ADC 鎵弿鈥斺€?4 澶╋紝浣犵殑鐘舵€佹満妗嗘灦宸叉垚鐔熷埌"濉┖鍗崇敤"鐨勭▼搴
