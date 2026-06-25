# 6.10 涓讳粠妯″紡 + PWM 淇″彿娴嬮噺

> **鑺墖**锛歋TM32F103C8T6 | **鐜**锛歏SCode + Keil + Keil Assistant  
> **涓婚**锛氫粠妯″紡澶嶄綅銆佷富妯″紡 TRGO銆佸弻瀹氭椂鍣ㄥ崗鍚屻€丳WM 鍛ㄦ湡/鍗犵┖姣旀祴閲?
---

## 涓€銆佹牳蹇冧唬鐮?
### 1. 浜х敓琚祴 PWM锛圱IM3_CH1 鈫?PA6锛?
```c
void Init_OC_generatePWM(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    // 鏃跺熀锛?ms 鍛ㄦ湡
    TIM_TimeBaseInitTypeDef tim = {0};
    tim.TIM_Prescaler = 71;     // 1MHz
    tim.TIM_Period    = 999;    // 1ms
    TIM_TimeBaseInit(TIM3, &tim);
    TIM_ARRPreloadConfig(TIM3, ENABLE);  // ARR 棰勮杞斤紝闃叉瘺鍒?
    // PWM 杈撳嚭
    TIM_OCInitTypeDef oc = {0};
    oc.TIM_OCMode      = TIM_OCMode_PWM1;
    oc.TIM_OCPolarity  = TIM_OCPolarity_High;
    oc.TIM_OutputState = TIM_OutputState_Enable;
    oc.TIM_Pulse       = 200;   // 20% 鍗犵┖姣?    TIM_OC1Init(TIM3, &oc);

    TIM_CCPreloadControl(TIM3, ENABLE);  // CCR 棰勮杞?    TIM_CtrlPWMOutputs(TIM3, ENABLE);
    TIM_Cmd(TIM3, ENABLE);
}
```

### 2. 娴嬮噺 PWM锛圱IM1 浠庢ā寮忓浣?+ 鍙岄€氶亾鎹曡幏锛?
```c
void Init_IC_measurePWM(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    // 鏃跺熀锛?渭s 鍒嗚鲸鐜?    TIM_TimeBaseInitTypeDef tim = {0};
    tim.TIM_Prescaler = 71;
    tim.TIM_Period    = 65535;
    TIM_TimeBaseInit(TIM1, &tim);

    // CH1锛氱洿鎺ヤ笂鍗囨部鎹曡幏锛堝崰绌烘瘮 = CCR2 - 0锛?    TIM_ICInitTypeDef ic = {0};
    ic.TIM_Channel    = TIM_Channel_1;
    ic.TIM_ICPolarity = TIM_ICPolarity_Rising;
    ic.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICInit(TIM1, &ic);

    // CH2锛氶棿鎺ヤ笅闄嶆部鎹曡幏
    ic.TIM_Channel    = TIM_Channel_2;
    ic.TIM_ICPolarity = TIM_ICPolarity_Falling;
    ic.TIM_ICSelection = TIM_ICSelection_IndirectTI;
    TIM_ICInit(TIM1, &ic);

    // 銆愬叧閿€戜粠妯″紡锛歍RGI 涓婂崌娌胯嚜鍔ㄦ竻闆?CNT
    TIM_SelectInputTrigger(TIM1, TIM_TS_TI1FP1);      // 瑙﹀彂婧?= CH1 婊ゆ尝鍚?    TIM_SelectSlaveMode(TIM1, TIM_SlaveMode_Reset);    // 澶嶄綅妯″紡

    TIM_Cmd(TIM1, ENABLE);
}
```

### 3. PWM 娴嬮噺鐘舵€佹満锛堣疆璇㈡爣蹇椾綅锛屾棤闇€ CC 涓柇锛侊級

```c
void Measure_PWM_state_machine(void)
{
    switch (measure_state)
    {
    case MS_INIT:
        TIM_SetCompare1(TIM3, 200);                    // 璁惧崰绌烘瘮 20%
        TIM_ClearFlag(TIM1, TIM_FLAG_Trigger | TIM_FLAG_CC1 | TIM_FLAG_CC2);
        ms_time = currentTick;
        measure_state = WAIT_TRIGGER;
        break;

    case WAIT_TRIGGER:
        if (TIM_GetFlagStatus(TIM1, TIM_FLAG_Trigger) == SET)
            measure_state = MS_PROCESS;                 // 瑙﹀彂鍒颁簡
        else if (currentTick - ms_time > 20)
            measure_state = INTER_VAL;                  // 瓒呮椂
        break;

    case MS_PROCESS:
        if (TIM_GetFlagStatus(TIM1, TIM_FLAG_CC2) == SET)  // 绛変笅闄嶆部
        {
            TIM_ClearFlag(TIM1, TIM_FLAG_CC2);
            ms_cc1 = TIM_GetCapture1(TIM1);             // 鍛ㄦ湡 (渭s)
            ms_cc2 = TIM_GetCapture2(TIM1);             // 楂樼數骞冲搴?(渭s)

            float period = ms_cc1 * 1.0e-6f * 1.0e3f;   // 渭s 鈫?ms
            float duty   = (float)ms_cc2 / ms_cc1 * 100.0f;  // %
            printf("Period=%.3fms  Duty=%.2f%%\r\n", period, duty);

            ms_time = currentTick;
            measure_state = INTER_VAL;
        }
        break;

    case INTER_VAL:
        if (currentTick - ms_time > 1000)                // 1s 娴嬩竴娆?            measure_state = MS_INIT;
        break;
    }
}
```

---

## 浜屻€佹牳蹇冪煡璇嗙偣

### 1. 浠庢ā寮?vs 涓绘ā寮?
| 鏂瑰悜 | 妯″紡 | 浣滅敤 |
|------|------|------|
| **浠庢ā寮?* | 鎺ュ彈澶栭儴瑙﹀彂锛圱RGI锛夋帶鍒惰鏁?| 澶嶄綅銆侀棬鎺с€佽Е鍙戙€佸閮ㄦ椂閽?|
| **涓绘ā寮?* | 閫氳繃 TRGO 杈撳嚭鍐呴儴浜嬩欢 | 鏇存柊銆佷娇鑳姐€佹瘮杈冭剦鍐层€丱CxREF |

> 6.10 鐢ㄧ殑浠庢ā寮忥細`TIM_SlaveMode_Reset` 鈥?TRGI 涓婂崌娌胯嚜鍔ㄦ竻闆?CNT

### 2. 涓轰粈涔堝浣嶆ā寮忕渷鎺変簡鍥炵粫璁＄畻

```
浠庢ā寮忓浣嶅墠锛?  CCR1 = 500, CCR2 = 300  鈫? 鑴夊 = 300, 浣嗗懆鏈?= ?锛堥渶璁＄畻 65536-500+...锛?
浠庢ā寮忓浣嶅悗锛?  姣忎釜 PWM 涓婂崌娌?鈫?CNT 鑷姩褰掗浂
  CCR1 = 鍛ㄦ湡(1000), CCR2 = 鑴夊(200)
  鍗犵┖姣?= CCR2 / CCR1锛屾棤闇€浠讳綍鍥炵粫澶勭悊锛?```

### 3. 淇″彿閾惧畬鏁磋矾寰?
```
PA6(TIM3_CH1 PWM) 鈹€鈹€鐗╃悊杩炵嚎鈹€鈹€鈫?PA8(TIM1_CH1)
                                    鈫?                          杈撳叆婊ゆ尝 鈫?杈规部妫€娴?涓婂崌娌?
                                    鈫?                    鈹屸啋 CH1 鐩存帴鎹曡幏 鈫?CCR1锛堝懆鏈燂級
                    鈹?                    鈹斺啋 TRGI(浠庢ā寮忓浣?CNT) + CH2 闂存帴鎹曡幏 鈫?CCR2锛堣剦瀹斤級
```

### 4. 涓轰綍涓嶉渶瑕?CC 涓柇锛?
- 浠庢ā寮忚嚜鍔ㄥ浣?CNT锛屼富寰幆鍙渶杞 `TIM_FLAG_Trigger` + `TIM_FLAG_CC2`
- 鐘舵€佹満鍦?`ms_init` 涓?`TIM_ClearFlag` 娓呮墍鏈夋畫浣欐爣蹇?- 涓诲惊鐜棤浠讳綍闃诲绛夊緟锛孋PU 鍗犵敤鏋佷綆

### 5. ARR 鍜?CCR 棰勮杞?
```c
TIM_ARRPreloadConfig(TIM3, ENABLE);   // ARR 褰卞瓙瀵勫瓨鍣?TIM_CCPreloadControl(TIM3, ENABLE);   // CCR 褰卞瓙瀵勫瓨鍣?```

- 杩愯鏃舵敼 ARR/CCR锛岀瓑鍒版洿鏂颁簨浠舵墠鐢熸晥
- 閬垮厤 PWM 杩愯涓敼鍙傛暟浜х敓姣涘埡

### 6. 浠庢ā寮?8 绉嶉€熸煡

| 妯″紡 | 浣滅敤 |
|------|------|
| **Reset** | TRGI 涓婂崌娌挎竻闆?CNT |
| Gated | TRGI 楂樻椂璁℃暟 |
| Trigger | TRGI 涓婂崌娌垮惎鍔ㄤ竴娆¤鏁?|
| External Clock 1 | TRGI 姣忎釜涓婂崌娌胯鏁颁竴娆?|

---

## 涓夈€佷唬鐮佽瘎浠?
- **浜偣**锛氱敤浠庢ā寮忓浣嶆浛浠ｈ蒋浠跺洖缁曡绠楋紝娣卞埢鐞嗚В浜?鐢ㄧ‖浠剁壒鎬х渷杞欢骞查"
- **鏋舵瀯**锛氬弻瀹氭椂鍣?+ 鍙岀姸鎬佹満 + 鏍囧織浣嶈疆璇紝CPU 鍑犱箮闆剁瓑寰?- **闃插尽鎬?*锛歚ms_init` 涓竻闄ゆ墍鏈夊彲鑳芥畫鐣欑殑鏍囧織浣嶏紝`ms_time=0` 闃查噹鍊?- **鍙紭鍖?*锛歚sin_flicker()` 鍦ㄤ富寰幆娴偣杩愮畻鐣ラ噸锛屽彲鐢ㄦ煡琛ㄦ硶鍔犻€?
---

## 鍥涖€佸績寰?
- 浠庢ā寮忓浣嶆槸杈撳叆鎹曡幏鐨?浣滃紛鐮?鈥斺€旂‖浠惰嚜鍔ㄦ竻闆?CNT锛屽洖缁曡绠楁垚涓哄巻鍙?- `TIM_SelectInputTrigger` vs `TIM_SelectOutputTrigger` 涓€瀛椾箣宸紝鏂瑰悜瀹屽叏涓嶅悓
- 杞鏍囧織浣?+ 鐘舵€佹満 鍙互瀹炵幇鏃犱腑鏂殑绮剧‘娴嬮噺鈥斺€斿墠鎻愭槸涓诲惊鐜蹇
