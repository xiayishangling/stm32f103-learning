# 6.16 FreeRTOS 闃熷垪瀹炴垬 鈥?涓柇鈫掗槦鍒椻啋浠诲姟

> **鑺墖**锛歋TM32F103C8T6 | **鐜**锛欳ubeMX + VSCode + Keil  
> **涓婚**锛歋RAM 浜旀鍒嗗尯銆佸懡鍚嶈鑼冦€佷换鍔″洓鎬併€侀槦鍒椾紶鍊?vs 浼犳寚閽?
---

## 涓€銆佹牳蹇冧唬鐮?
### 1. 鎸夐敭娑堟姈锛圧TOS 闈為樆濉炵増锛?
```c
// 闈欐€佸眬閮ㄥ彉閲忎繚瀛樿鏁板櫒+鎸夊帇鐘舵€侊紝10ms 鎵弿涓€娆?uint8_t isClickBtn(void)
{
    static uint32_t count = 0;
    static uint32_t pressed = 0;

    if (IS_KEY_PRESSED() && !pressed) {
        count++;
        if (count >= KEY_DEBOUNCE_COUNT && IS_KEY_PRESSED()) {
            pressed = 1;
            return 1;   // 纭涓€娆″嚮閿?        }
    }
    if (!IS_KEY_PRESSED()) {
        count = 0;
        pressed = 0;     // 鏉惧紑鍚庡叏閮ㄦ竻闆?    }
    return 0;
}
```

### 2. 闃熷垪浼犳寚閽堬細鎸夐敭 鈫?LED 鎺у埗

```c
typedef enum  { LEDColor_Red, LEDColor_Blue } LED_Color;
typedef enum  { LEDState_ON = 1, LEDState_OFF = 0 } LED_State;
typedef struct { LED_Color color; LED_State state; } LED_Message;

// 鎸夐敭浠诲姟锛氬姩鎬佸垎閰嶆秷鎭?鈫?闃熷垪
void vBtnTask(void *argument)
{
    LED_State state = LEDState_OFF;
    for (;;) {
        if (isClickBtn()) {
            state = (state == LEDState_OFF) ? LEDState_ON : LEDState_OFF;
            LED_Message *msg = pvPortMalloc(sizeof(LED_Message));
            if (msg) {
                msg->color = LEDColor_Red;
                msg->state = state;
                if (osMessageQueuePut(LEDQueueHandle, &msg, 0, osWaitForever) != osOK)
                    vPortFree(msg);   // 闃熷垪婊″垯閲婃斁锛岄伩鍏嶆硠婕?            }
        }
        osDelay(10);
    }
}

// LED 浠诲姟锛氬彇娑堟伅 鈫?鍐?GPIO 鈫?閲婃斁鍐呭瓨
void vLEDTask(void *argument)
{
    for (;;) {
        LED_Message *msg;
        if (osMessageQueueGet(LEDQueueHandle, &msg, 0, 0) == osOK) {
            GPIO_WritePin(..., msg->state ? SET : RESET);
            vPortFree(msg);   // 澶勭悊瀹屽繀椤婚噴鏀撅紒
        }
    }
}
```

### 3. 涓柇鍥炶皟 鈫?闃熷垪 鈫?鍛戒护浠诲姟

```c
uint8_t rxData;

// 鍚姩涓柇鎺ユ敹
void UART1_Receive_Start(void)
    { HAL_UART_Receive_IT(&huart1, &rxData, 1); }

// 鎺ユ敹瀹屾垚鍥炶皟 鈫?鍙姇閫掗槦鍒楋紝绔嬪嵆閲嶅惎鎺ユ敹
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        osMessageQueuePut(CommandQueueHandle, &rxData, 0, 0);
        HAL_UART_Receive_IT(&huart1, &rxData, 1);
    }
}

// 鍛戒护浠诲姟锛氫粠闃熷垪鍙栧瓧鑺?鈫?瑙ｆ瀽鎵ц
void vCommandTask(void *argument)
{
    UART1_Receive_Start();
    uint8_t byte;
    for (;;) {
        if (osMessageQueueGet(CommandQueueHandle, &byte, 0, osWaitForever) == osOK) {
            switch (byte) {
                case '1': LED1_Toggle(); break;
                case '2': LED2_ON();     break;
                case '3': LED1_OFF(); LED2_OFF(); break;  // 鈿狅笍 蹇呴』鍔?break
            }
        }
    }
}
```

---

## 浜屻€佹牳蹇冪煡璇嗙偣

### 1. SRAM 浜旀鍒嗗尯

| 娈?| 瀛樺偍鍐呭 | 澧為暱鏂瑰悜 | 璇存槑 |
|----|----------|----------|------|
| `.data` | 闈為浂鍒濆€煎叏灞€/闈欐€佸彉閲?| 鈥?| 鍒濆€煎湪 Flash锛屽惎鍔ㄦ椂澶嶅埗 |
| `.bss` | 闆跺垵鍊?鏃犲垵鍊煎叏灞€/闈欐€?| 鈥?| 鍚姩鑷姩娓呴浂锛屼笉鍗?Flash |
| `.heap` | `malloc` 鍔ㄦ€佸垎閰?| 鈫?鍚戦珮鍦板潃 | FreeRTOS 鍐呮牳瀵硅薄鍦ㄦ |
| (绌洪棽) | 鈥?| 鈥?| 鍫嗘爤鐩告挒 = 鍐呭瓨婧㈠嚭 |
| `.stack` | 灞€閮ㄥ彉閲?鍙傛暟/杩斿洖鍦板潃 | 鈫?鍚戜綆鍦板潃 | 姣忎换鍔＄嫭绔嬫爤 |

### 2. FreeRTOS 鍛藉悕瑙勮寖

| 鍓嶇紑 | 绫诲瀷 | 绀轰緥 |
|------|------|------|
| `c` | char | `cCount` |
| `s` | short | `sValue` |
| `l` | long | `lGlobalVar` |
| `x` | BaseType_t/鍙ユ焺 | `xQueue` |
| `e` | enum | `eState` |
| `p` | 鎸囬拡 | `pxTask` |
| `u` | unsigned | `uxPriority` |
| 鍙犲姞 | `puc` = uint8_t* | `pucBuffer` |

鍑芥暟锛歚杩斿洖鍊煎墠缂€ + 妯″潡鍚?+ 鍔熻兘鍚峘锛堝 `xTaskCreate`锛? 
闈欐€佸嚱鏁帮細`prv` 寮€澶达紙private锛? 
瀹忥細`鏂囦欢鍓嶇紑灏忓啓_澶у啓涓讳綋`锛堝 `pdTRUE`銆乣configMAX_PRIORITIES`锛?
### 3. 浠诲姟鍥涙€佹祦杞?
```
鍒涘缓 鈫?灏辩华 鈬?杩愯 鈫?闃诲锛堜富鍔ㄧ瓑锛夆啋 灏辩华
                鈫?鎸傝捣锛堣鍔ㄥ喕缁擄級鈫?鎭㈠ 鈫?灏辩华
```

| 鐘舵€?| 鐗圭偣 | 杩涘叆鏂瑰紡 |
|------|------|----------|
| 灏辩华 | 涓囦簨淇卞锛岀瓑 CPU | 鍒涘缓/闃诲鍒版湡/鎭㈠ |
| 杩愯 | 姝ｅ湪鎵ц锛堝崟鏍稿敮涓€锛?| 璋冨害鍣ㄩ€変腑 |
| 闃诲 | 涓诲姩浼戠湢锛屼笉鍗?CPU | `vTaskDelay`/绛夐槦鍒?|
| 鎸傝捣 | 琚姩鍐荤粨锛岄渶鎵嬪姩鎭㈠ | `vTaskSuspend` |

### 4. 闃熷垪浼犲€?vs 浼犳寚閽?
| 鏂瑰紡 | 閫傜敤 | 娉ㄦ剰 |
|------|------|------|
| **鎸夊€?* | 灏忔暟鎹紙瀛楄妭/鏁村瀷锛?| 鐩存帴鎷疯礉锛岀畝鍗曞畨鍏?|
| **鎸夋寚閽?* | 缁撴瀯浣?澶у潡鏁版嵁 | 鐢?`pvPortMalloc`锛屾帴鏀舵柟蹇呴』 `vPortFree` |

> 鈿狅笍 **甯歌閿欒**锛氫紶灞€閮ㄥ彉閲忓湴鍧€銆傚眬閮ㄥ彉閲忓嚭浣滅敤鍩熷嵆閿€姣侊紝鎺ユ敹鏂硅鍒伴噹鎸囬拡銆?
### 5. 涓柇鈫掗槦鍒椻啋浠诲姟 缁忓吀妯″紡

```
ISR锛堟瀬鐭級          浠诲姟锛堣€楁椂澶勭悊锛?  鈹?                   鈹?  鈹?鏀舵暟鎹?            鈹?  鈹?QueuePut 鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈫?鈹?QueueGet
  鈹?閲嶅惎涓柇鎺ユ敹       鈹?瑙ｆ瀽/鎵ц
```

涓柇鍙仛涓や欢浜嬶細**鏀舵暟鎹?+ 鎶曢槦鍒?*銆傝繖鏄?RTOS 瀹炴椂鎬х殑鏍瑰熀銆?
---

## 涓夈€佸績寰?
- SRAM 浜旀鍒嗗尯鏄悊瑙?鏍堟孩鍑?"鍐呭瓨娉勬紡"鐨勫墠鎻愨€斺€旈潰璇曞繀闂?- FreeRTOS 鍛藉悕瑙勮寖鐪嬩技绻佺悙锛屽疄鍒欐槸璇绘簮鐮佺殑閽ュ寵鈥斺€擿prv`銆乣pd`銆乣config` 瑙佸悕鐭ユ簮
- 闃熷垪浼犳寚閽堟椂 `pvPortMalloc` 鈫?`vPortFree` 鎴愬鍑虹幇锛屾紡涓€涓氨鏄唴瀛樻硠婕?- 涓柇鍥炶皟閲屽彧鎶曢槦鍒椼€佷笉澶勭悊涓氬姟锛屾槸 RTOS 涓庤８鏈烘渶澶х殑鎬濈淮杞彉
