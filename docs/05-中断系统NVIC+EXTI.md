# 6.5 涓柇绯荤粺 鈥?NVIC + EXTI

> **鑺墖**锛歋TM32F103C8T6 | **鐜**锛歏SCode + Keil + Keil Assistant  
> **涓婚**锛歂VIC 浼樺厛绾у垎缁勩€乁SART 涓柇銆丒XTI 澶栭儴涓柇銆佽疆璇笌涓柇鍐茬獊

---

## 涓€銆佹牳蹇冧唬鐮?
### 1. NVIC 閰嶇疆 + USART 涓柇

```c
void init_NVIC(void)
{
    // 涓嶉渶瑕佸紑鏃堕挓锛孨VIC 灞炰簬 Cortex-M 鍐呮牳
    NVIC_InitTypeDef nvic = {0};
    nvic.NVIC_IRQChannel                   = USART1_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 0;   // 鎶㈠崰浼樺厛绾?0
    nvic.NVIC_IRQChannelSubPriority        = 0;   // 瀛愪紭鍏堢骇 0
    nvic.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nvic);
}

// USART 鍒濆鍖栦腑鎵撳紑 RXNE 涓柇
USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
```

### 2. USART 涓柇鏈嶅姟鍑芥暟

```c
void USART1_IRQHandler(void)
{
    if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET)
    {
        uint16_t ch = USART_ReceiveData(USART1);
        if (ch == '1')      Flicker_time = 500;   // 鎱㈤棯
        else if (ch == '2') Flicker_time = 50;    // 蹇棯
        else if (ch == '0') {
            // 闃诲绛夊緟鎸夐敭锛堜粎婕旂ず锛屽疄闄呭簲閬垮厤鍦?ISR 涓绛夛級
            while (1) {
                if (KEY_PRESSED) { /* 娑堟姈+閲婃斁 */ break; }
            }
        }
    }
}

// 涓诲惊鐜細LED 浠?Flicker_time 鍛ㄦ湡闂儊锛屼覆鍙ｄ腑鏂姩鎬佹敼閫熷害
void Process_NVIC_Control_LedFlicker(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  // 鍏ㄥ眬鍙皟涓€娆?    init_NVIC_PC13_Gpio(); init_NVIC_Usart_Gpio();
    init_NVIC_Usart(); init_NVIC();
    while (1) { LED_flicker(); }
}
```

### 3. EXTI 閰嶇疆锛堜笂鍗囨部瑙﹀彂锛?
```c
void init_EXTI(void)
{
    // 鈶?缁戝畾 GPIO 寮曡剼鍒?EXTI 绾?    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource1);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource2);

    // 鈶?閰嶇疆 NVIC锛圗XTI1 鍜?EXTI2 鍚勬湁鐙珛 IRQ 閫氶亾锛?    NVIC_InitTypeDef nvic = {0};
    nvic.NVIC_IRQChannelPreemptionPriority = 0;
    nvic.NVIC_IRQChannelSubPriority        = 0;
    nvic.NVIC_IRQChannelCmd                = ENABLE;

    nvic.NVIC_IRQChannel = EXTI1_IRQn;  NVIC_Init(&nvic);
    nvic.NVIC_IRQChannel = EXTI2_IRQn;  NVIC_Init(&nvic);

    // 鈶?閰嶇疆 EXTI
    EXTI_InitTypeDef exti = {0};
    exti.EXTI_Line    = EXTI_Line1 | EXTI_Line2;  // 浣嶆帺鐮侊紝鍙?| 鍚堝苟
    exti.EXTI_Mode    = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = EXTI_Trigger_Rising;       // 鏉炬墜瑙﹀彂
    exti.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti);
}
```

### 4. EXTI 涓柇鏈嶅姟鍑芥暟

```c
void EXTI1_IRQHandler(void)
{
    if (EXTI_GetFlagStatus(EXTI_Line1) == SET) {
        EXTI_ClearFlag(EXTI_Line1);                     // 鍏堟竻鏍囧織
        GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);   // 鏉炬墜浜伅
    }
}

void EXTI2_IRQHandler(void)
{
    if (EXTI_GetFlagStatus(EXTI_Line2) == SET) {
        EXTI_ClearFlag(EXTI_Line2);
        GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);     // 鏉炬墜鐏伅
    }
}
```

---

## 浜屻€佹牳蹇冪煡璇嗙偣

### 1. NVIC 浼樺厛绾т綋绯?
```
NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
```

| 鍒嗙粍 | 鎶㈠崰浣嶆暟 | 瀛愪紭鍏堢骇浣嶆暟 | 鎶㈠崰鑼冨洿 | 瀛愯寖鍥?|
|------|----------|--------------|----------|--------|
| 0 | 0 | 4 | 鈥?| 0~15 |
| 1 | 1 | 3 | 0~1 | 0~7 |
| **2** | **2** | **2** | **0~3** | **0~3** |
| 3 | 3 | 1 | 0~7 | 0~1 |
| 4 | 4 | 0 | 0~15 | 鈥?|

**浠茶瑙勫垯**锛?1. 鎶㈠崰浼樺厛绾ф暟鍊煎皬鐨勫厛鎵ц
2. 鎶㈠崰鐩稿悓 鈫?姣斿瓙浼樺厛绾?3. 閮界浉鍚?鈫?鎸?IRQ 缂栧彿锛堝皬鐨勫厛鍝嶅簲锛?4. 楂樻姠鍗犵骇鍙?*宓屽鎵撴柇**浣庢姠鍗犵骇锛屽瓙浼樺厛绾т笉鑳芥墦鏂悓鎶㈠崰绾?
> `NVIC_PriorityGroupConfig()` 鍏ㄥ眬鍙皟涓€娆★紝鍦ㄦ墍鏈変腑鏂垵濮嬪寲涔嬪墠

### 2. IRQn锛堢储寮曪級vs 浣嶆帺鐮?
| 绫诲瀷 | 鍙惁 `|` 鍚堝苟 | 渚嬪瓙 |
|------|:--:|------|
| **IRQn**锛堜腑鏂€氶亾鍙凤級 | 鉂?| `USART1_IRQn` = 37锛屾槸绱㈠紩涓嶆槸浣?|
| **GPIO_Pin / EXTI_Line** | 鉁?| `EXTI_Line1 \| EXTI_Line2` = 0x00006 |

> 閰嶇疆澶氫釜 NVIC 閫氶亾鏃堕渶澶氭璋冪敤 `NVIC_Init()`锛屼笉鑳界敤 `|` 鍚堝苟 IRQn

### 3. NVIC vs GPIO 缁撴瀯浣撳樊寮?
| 澶栬 | 缁撴瀯浣?| 鍘熷洜 |
|------|--------|------|
| **GPIO / USART / SPI** | 鏈夐璁惧叏灞€缁撴瀯浣?| ST 鍦ㄥ簱涓畾涔変簡 `GPIOA`銆乣USART1` 绛?|
| **NVIC** | 闇€鐢ㄦ埛鑷鍒涘缓 | 閬靛惊 ARM CMSIS 鏍囧噯锛屽彧鎻愪緵绫诲瀷瀹氫箟 |

### 4. EXTI 閰嶇疆瀹屾暣姝ラ

```
鈶?寮€ AFIO 鏃堕挓
鈶?GPIO_EXTILineConfig(绔彛, 寮曡剼婧?   鈥?缁戝畾 GPIO 鍒?EXTI 绾?鈶?NVIC_Init 閰嶇疆瀵瑰簲 IRQ 閫氶亾
鈶?EXTI_Init锛圠ine/Mode/Trigger/Cmd锛?鈶?ISR 涓厛 EXTI_GetFlagStatus 纭锛屽啀 EXTI_ClearFlag 娓呴櫎
```

### 5. EXTI 绾垮彿涓?NVIC 閫氶亾鏄犲皠

| EXTI 绾?| IRQ 閫氶亾 | 璇存槑 |
|---------|----------|------|
| Line0~4 | `EXTI0_IRQn` ~ `EXTI4_IRQn` | 鍚勮嚜鐙珛 |
| Line5~9 | `EXTI9_5_IRQn` | **5 鏉＄嚎鍏辩敤涓€涓€氶亾** |
| Line10~15 | `EXTI15_10_IRQn` | 6 鏉＄嚎鍏辩敤涓€涓€氶亾 |

### 6. 杞涓庝腑鏂啿绐侊紙閲嶈锛侊級

```c
// 鉂?閿欒锛氫富寰幆杞 + EXTI 涓柇鍚屾椂鎺у悓涓€涓?LED
while (1) {
    EXTI_Button_Control_PC13();  // 杞锛氭寜涓嬬珛鍒诲彉
}
// ISR 涓澗鎵嬪張鍙樹竴娆?鈫?琛屼负娣蜂贡

// 鉁?姝ｇ‘锛氬彧淇濈暀涓€涓喅绛栨簮
while (1) {
    // 娉ㄩ噴鎺夎疆璇紝绾腑鏂帶鍒?}
```

> 鍚屼竴璧勬簮鍙兘鏈変竴涓?鑰佹澘"鈥斺€旇涔堣疆璇€佽涔堜腑鏂紝鎴栫敤鏍囧織浣嶉€氫俊

### 7. 纭欢娴佹帶锛圧TS/CTS锛?
| 绫诲瀷 | 鏈哄埗 | 绫绘瘮 |
|------|------|------|
| UART RTS/CTS | 棰濆 2 绾匡紝鎺ユ敹鏂瑰揩婊℃椂鎷?RTS | 甯﹀娴佹帶锛堝鎺ョ孩鐏級 |
| I2C 鏃堕挓鎷変几 | 浠庢満鎷変綆 SCL | 甯﹀唴娴佹帶锛堝崗璁唴缃級 |

> 鏃ュ父浣跨敤閫氬父鍏抽棴锛歚USART_HardwareFlowControl_None`

---

## 涓夈€佸績寰?
- NVIC 鐨?鏁板€艰秺灏忚秺浼樺厛"鏄弽鐩磋鐨勶紝闇€瑕佸弽澶嶈蹇?- IRQn 鏄储寮曘€丒XTI_Line 鏄綅鎺╃爜鈥斺€旀贩娣嗚繖涓細瀵艰嚧缂栬瘧閫氳繃浣嗚涓哄紓甯?- EXTI 鐨?AFIO 缁戝畾鏄鏄撹璺宠繃鐨勬楠わ細**鍏?GPIO_EXTILineConfig锛屽啀 EXTI_Init**
- 杞+涓柇鍚屾椂鎶竴涓?LED 鐨勬暀璁潪甯告繁鍒烩€斺€旇繖鏄祵鍏ュ紡骞跺彂鍐茬獊鐨勭缉褰?- ISR 搴旂煭灏忕簿鎮嶏紝姝荤瓑鎸夐敭铏界劧鑳借窇浣嗕笉鏄渶浣冲疄璺碉紙鍚庣画鐢ㄧ姸鎬佹満鏀硅繘锛
