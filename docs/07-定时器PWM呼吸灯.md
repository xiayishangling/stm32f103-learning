# 6.7 瀹氭椂鍣?鈥?鏃跺熀鍗曞厓 + PWM 鍛煎惛鐏?
> **鑺墖**锛歋TM32F103C8T6 | **鐜**锛歏SCode + Keil + Keil Assistant  
> **涓婚**锛氭椂鍩哄崟鍏冦€侀潪闃诲寤舵椂銆佷簰琛?PWM 杈撳嚭銆佹寮﹀懠鍚哥伅

---

## 涓€銆佹牳蹇冧唬鐮?
### 1. TIM2 涓柇椹卞姩姣璁℃暟鍣紙鏇夸唬 GetTick锛?
```c
volatile uint32_t currentTick = 0;  // volatile锛氫腑鏂拰涓诲惊鐜兘璁块棶

void Init_TIM2(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseInitTypeDef tim = {0};
    tim.TIM_Prescaler         = 71;              // 72M / (71+1) = 1MHz = 1渭s
    tim.TIM_Period            = 999;             // 1渭s 脳 1000 = 1ms
    tim.TIM_CounterMode       = TIM_CounterMode_Up;
    tim.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &tim);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);   // 浣胯兘鏇存柊涓柇
    NVIC_Init(/* TIM2_IRQn */);
    TIM_Cmd(TIM2, ENABLE);
}

// ISR锛氭瘡 1ms 瑙﹀彂涓€娆?void TIM2_IRQHandler(void)
{
    if (TIM_GetFlagStatus(TIM2, TIM_IT_Update) == SET) {
        TIM_ClearFlag(TIM2, TIM_IT_Update);
        currentTick++;  // 鍏堟竻鏍囧織锛屽啀鎿嶄綔
    }
}
```

### 2. 闈為樆濉炲欢鏃讹紙鍙栦唬 Delay锛?
```c
void My_Delay_1(uint32_t ms)
{
    uint32_t expire = currentTick + ms;
    while (currentTick < expire);  // 蹇欑瓑浣嗕笉闃诲涓柇
}

// 浣跨敤鐘舵€佹満瀹炵幇 1s 闂儊锛堝畬鍏ㄤ笉鍗?CPU锛?void Process_mydelay(void)
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

### 3. TIM1 浜掕ˉ PWM + 姝ｅ鸡鍛煎惛鐏?
```c
void init_CCR1_Compare(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    // 鏃跺熀锛?ms 鍛ㄦ湡
    TIM_TimeBaseInitTypeDef tim = {0};
    tim.TIM_Prescaler = 71;   // 1MHz
    tim.TIM_Period    = 999;  // 1ms
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM1, &tim);

    TIM_Cmd(TIM1, ENABLE);
    TIM_CtrlPWMOutputs(TIM1, ENABLE);  // MOE 鈥?楂樼骇瀹氭椂鍣ㄥ繀椤诲紑锛?
    // 杈撳嚭姣旇緝锛欳H1(PA8) + CH1N(PB13) 浜掕ˉ
    TIM_OCInitTypeDef oc = {0};
    oc.TIM_OCMode      = TIM_OCMode_PWM1;
    oc.TIM_OCPolarity  = TIM_OCPolarity_High;
    oc.TIM_OCNPolarity = TIM_OCPolarity_High;     // 浜掕ˉ鏋佹€т笌 CH1 鐩稿悓 鈫?浜ゆ浛浜?    oc.TIM_OutputState  = TIM_OutputState_Enable;
    oc.TIM_OutputNState = TIM_OutputNState_Enable;
    oc.TIM_Pulse        = 0;                      // 鍒濆鍗犵┖姣?0
    TIM_OC1Init(TIM1, &oc);
}

// 涓诲惊鐜細姝ｅ鸡鍔ㄦ€佸崰绌烘瘮
void Process_Breathing_lamp(void)
{
    init_CCR1_Compare();
    Init_TIM2();

    while (1)
    {
        float t    = currentTick * 1.0e-3f;               // ms 鈫?s
        float duty = 0.5f * (sinf(2.0f * 3.1416f * t) + 1); // 0~1 姝ｅ鸡
        uint16_t ccr = (uint16_t)(duty * 1000.0f);        // CCR = 鍗犵┖姣?脳 鍛ㄦ湡
        TIM_SetCompare1(TIM1, ccr);                       // 鍔ㄦ€佹洿鏂?CCR
    }
}
```

---

## 浜屻€佹牳蹇冪煡璇嗙偣

### 1. 鏃跺熀鍗曞厓鍥涜绱?
| 瀵勫瓨鍣?| 浣滅敤 | 鍏紡 |
|--------|------|------|
| **PSC** | 棰勫垎棰戯紝闄嶄綆杈撳叆鏃堕挓 | `f_tim = f_in / (PSC+1)` |
| **ARR** | 鑷姩閲嶈锛屽喅瀹氬懆鏈?| `鍛ㄦ湡 = ARR + 1` |
| **CNT** | 璁℃暟鍣紝瀵瑰垎棰戝悗鑴夊啿璁℃暟 | 鍒?ARR 鍚庡綊闆?|
| **RCR** | 閲嶅璁℃暟鍣?| `N 娆℃孩鍑?= RCR + 1` |

> 渚嬶細72MHz 鈫?PSC=71 鈫?1MHz 鈫?ARR=999 鈫?1ms 涓柇

### 2. 褰卞瓙瀵勫瓨鍣紙棰勮杞斤級

PSC/ARR/RCR 閮芥湁褰卞瓙瀵勫瓨鍣ㄣ€傚啓鍏ュ悗绛夊埌**鏇存柊浜嬩欢**鎵嶇敓鏁堬紝闃叉 PWM 杩愯涓敼鍙傛暟浜х敓姣涘埡銆侫RR 棰勮杞介渶鎵嬪姩寮€鍚?`ARPE`銆?
### 3. PWM 鍗犵┖姣斿叕寮?
```
CCR = 鍗犵┖姣?0~1) 脳 (ARR + 1)
```

PWM 妯″紡 1锛欳NT < CCR 鈫?鏈夋晥鐢靛钩锛汣NT 鈮?CCR 鈫?鏃犳晥鐢靛钩

### 4. 楂樼骇瀹氭椂鍣?MOE

```c
TIM_CtrlPWMOutputs(TIM1, ENABLE);  // 蹇呴』璋冪敤锛屽惁鍒欏紩鑴氭棤娉㈠舰
```

TIM1/TIM8 鏄珮绾у畾鏃跺櫒锛屾湁鍒硅溅/姝诲尯鍔熻兘锛孧OE锛堜富杈撳嚭浣胯兘锛夋槸鎬诲紑鍏炽€?
### 5. 浜掕ˉ杈撳嚭

- CHx 鍜?CHxN 鐢靛钩鐩稿弽锛堢粡杩囧弽鐩稿櫒鐨勮瘽锛?- 鑻ユ瀬鎬ц鐩稿悓锛坄High` + `High`锛夛紝鍒?CHxN 鑷姩鍙嶇浉
- 姝诲尯鏃堕棿鐢?`TIM_BDTRInitTypeDef` 閰嶇疆

### 6. Cortex-M3 娴偣浼樺寲

```c
// 鉂?鍙岀簿搴︼紙Cortex-M3 鏃犵‖浠?FPU锛屾瀬鎱級
float duty = 0.5 * (sin(2.0 * 3.1415926 * t) + 1);

// 鉁?鍗曠簿搴?float duty = 0.5f * (sinf(2.0f * 3.1416f * t) + 1);
```

`f` 鍚庣紑 + `sinf` 閬垮厤鍙岀簿搴﹁蒋娴偣寮€閿€銆?
### 7. Goto 浣跨敤杈圭晫

| 鉁?鍙敤 | 鉂?绂佹 |
|---------|---------|
| 鍚戝悗璺冲埌缁熶竴娓呯悊/閿欒澶勭悊 | 鍚戝墠璺宠浆 |
| 璺冲嚭澶氶噸寰幆 | 浜ゅ弶閫昏緫 |
| Linux 鍐呮牳甯歌妯″紡 | 浣滀负甯歌鎺у埗娴?|

### 8. 绯荤粺閿欒澶勭悊鐘舵€佹満

```c
// 閿欒鐮?鈫?鏍囧織浣?鈫?涓诲惊鐜檷绾у鐞?if (system_err_flag) {
    switch (System_State) {
        case SYSTEM_HSE_TIMEOUT:  /* 澶勭悊 */ break;
        case SYSTEM_PLL_TIMEOUT:  /* 澶勭悊 */ break;
        case SYSTEM_SELECT_ERR:   /* 鍒囧洖 HSI */ break;
    }
}
```

> 閬垮厤 `while(1)` 姝荤瓑锛岃绯荤粺鏈夎嚜鎰堣兘鍔?
---

## 涓夈€佸績寰?
- 瀹氭椂鍣ㄤ腑鏂┍鍔ㄧ殑 `currentTick` 鏄８鏈烘椂闂寸郴缁熺殑鏍囬厤鈥斺€旀瘮 `GetTick` 鏇村簳灞傘€佹洿鍙帶
- PWM 鍛煎惛鐏槸鎰熷畼涓婃渶"鐪嬪緱瑙?鐨勬垚灏憋細鍏紡 鈫?瀵勫瓨鍣?鈫?娉㈠舰 鈫?鐏厜锛岄棴鐜劅鏋佸己
- `TIM_CtrlPWMOutputs` 鏄珮绾у畾鏃跺櫒鐨?闅愯棌寮€鍏?锛屽繕寮€鏄粡鍏稿潙
- 娴偣鏁?`f` 鍚庣紑鍦?Cortex-M3 涓婁笉鏄亸濂芥槸蹇呴渶
