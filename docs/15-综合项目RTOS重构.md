# 6.18 缁煎悎椤圭洰 FreeRTOS 閲嶆瀯 鈥?浜掓枼閿?+ 璁℃暟淇″彿閲?
> **鑺墖**锛歋TM32F103C8T6 | **鐜**锛欳ubeMX + VSCode + Keil  
> **涓婚**锛氳８鏈虹患鍚堥」鐩啋RTOS閲嶆瀯銆佷簰鏂ラ攣銆佽鏁颁俊鍙烽噺銆?浠诲姟绠￠亾

---

## 涓€銆佹牳蹇冧唬鐮?
### 1. 鍏ㄥ眬鐘舵€?+ 浜掓枼閿佷繚鎶?OLED

```c
// 缁撴瀯浣撻泦涓鐞嗗叏灞€鐘舵€?typedef struct {
    LED_State    led;
    OLED_State   oled;
    W25Q64_State w25q64;
} AppState;

volatile AppState g_state = { LED_State_OFF, OLED_State_Display3, W25Q64_State_Save };

// LED 浠诲姟锛氫簰鏂ラ攣淇濇姢 OLED 鍒锋柊
void vIP_LEDTask(void *argument)
{
    for (;;) {
        USARTMessage msg;
        if (osMessageQueueGet(USART_QueueHandle, &msg, 0, osWaitForever) == osOK) {
            g_state.led  = msg.led_state;
            g_state.oled = msg.oled_state;

            // OLED 鏄叡浜‖浠?鈫?涓婇攣
            osMutexAcquire(General_MutexHandle, osWaitForever);
            OLED_Show_LED_Status(msg.led_state);  // 鍒锋柊灞忓箷
            osMutexRelease(General_MutexHandle);
        }
    }
}
```

### 2. 璁℃暟淇″彿閲忥細涓插彛鎺ユ敹涓嶄涪鏁版嵁

```c
// 涓柇鍥炶皟锛氭瘡鏀朵竴涓瓧鑺?鈫?Release锛堣鏁?1锛?void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        osSemaphoreRelease(USART_CountingSemHandle);
        HAL_UART_Receive_IT(&huart1, &rxData, 1);  // 绔嬪嵆閲嶅惎鎺ユ敹
    }
}

// 涓插彛浠诲姟锛欰cquire锛堣鏁?1锛夆啋 澶勭悊锛岀洿鍒拌鏁板綊闆?void vIP_USARTTask(void *argument)
{
    UART1_Receive_Start();
    char cmdbuff[8]; uint16_t idx = 0;
    for (;;) {
        osSemaphoreAcquire(USART_CountingSemHandle, osWaitForever);
        uint8_t byte = rxData;
        if (byte == '\r' || byte == '\n') {
            if (idx > 0) {
                cmdbuff[idx] = '\0';
                if      (strcmp(cmdbuff, "1") == 0) usart.led_state = LED_State_ON;
                else if (strcmp(cmdbuff, "0") == 0) usart.led_state = LED_State_OFF;
                else if (strcmp(cmdbuff, "2") == 0) usart.led_state = LED_State_Flicker;
                osMessageQueuePut(USART_QueueHandle, &usart, 0, 0);
                idx = 0;
            }
        } else { cmdbuff[idx++] = byte; }
    }
}
```

### 3. 鎸夐敭 鈫?闃熷垪 鈫?W25Q64 浠诲姟

```c
// 涓柇鍥炶皟锛氫袱涓寜閿叡鐢紝鏋佺畝
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == PA1_KEY1_Pin || GPIO_Pin == PA2_KEY2_Pin)
        osSemaphoreRelease(KEY_BinarySemHandle);  // 鍙噴鏀撅紝涓嶅鐞?}

// 鎸夐敭浠诲姟锛氭秷鎶?鈫?璇嗗埆 鈫?鍏ラ槦
void vIP_KEYTask(void *argument)
{
    for (;;) {
        osSemaphoreAcquire(KEY_BinarySemHandle, osWaitForever);
        vTaskDelay(pdMS_TO_TICKS(20));  // 杞欢娑堟姈
        KEYMessage msg;
        if (!HAL_GPIO_ReadPin(PA1_KEY1_Port, PA1_KEY1_Pin))  msg.w25q64_state = W25Q64_State_Save;
        else if (!HAL_GPIO_ReadPin(PA2_KEY2_Port, PA2_KEY2_Pin)) msg.w25q64_state = W25Q64_State_Recovery;
        else continue;
        osMessageQueuePut(KEY_QueueHandle, &msg, 0, 0);
    }
}

// W25Q64 浠诲姟锛氬嚭闃?鈫?SPI 淇濆瓨/璇诲彇 鈫?鏇存柊鍏ㄥ眬鐘舵€?void vIP_W25Q64Task(void *argument)
{
    for (;;) {
        KEYMessage msg;
        if (osMessageQueueGet(KEY_QueueHandle, &msg, 0, osWaitForever) == osOK) {
            if (msg.w25q64_state == W25Q64_State_Save) {
                uint8_t cur = HAL_GPIO_ReadPin(LED_Port, LED_Pin);
                W25Q64_Save_State(cur ? 0x01 : 0x00);
            } else {
                uint8_t rcvy = W25Q64_Read_ME();
                if (rcvy == 0x01)      { g_state.led = LED_State_ON;  g_state.oled = OLED_State_Display1; }
                else if (rcvy == 0x00) { g_state.led = LED_State_OFF; g_state.oled = OLED_State_Display2; }
            }
        }
        osDelay(10);
    }
}
```

---

## 浜屻€佹牳蹇冪煡璇嗙偣

### 1. 浜掓枼閿?vs 浜岃繘鍒朵俊鍙烽噺

| 鐗规€?| 浜掓枼閿?| 浜岃繘鍒朵俊鍙烽噺 |
|------|--------|-------------|
| 鐢ㄩ€?| 淇濇姢**鍏变韩璧勬簮**浜掓枼 | **浜嬩欢鍚屾**閫氱煡 |
| 鎵€鏈夋潈 | **璋侀攣璋佽В** | 浠绘剰浠诲姟鍙?Give/Take |
| 浼樺厛绾х户鎵?| 鉁?鏈?| 鉂?鏃?|
| 鍏稿瀷鍦烘櫙 | OLED/SPI 鍏变韩纭欢 | 涓柇閫氱煡浠诲姟 |

### 2. 璁℃暟淇″彿閲忥細姣斾簩杩涘埗鏇村彲闈?
```
浜岃繘鍒讹細鍙兘璁板綍"鏈夎繃浜嬩欢" 鈫?浠诲姟鏉ヤ笉鍙婂鐞?= 涓㈠け
璁℃暟锛? 姣忔潵涓€涓簨浠惰鏁?1 鈫?浠诲姟閫愪釜澶勭悊锛屼笉涓?```

> 涓插彛杩炵画鎺ユ敹鏄渶鍏稿瀷鐨勮鏁颁俊鍙烽噺鍦烘櫙

### 3. 浠诲姟绠￠亾璁捐锛? 浠诲姟锛?
```
涓柇(鏋佺煭)          浠诲姟(涓氬姟閫昏緫)
  鈹?                   鈹?  鈹?USART涓柇 鈹€鈹€Sem鈹€鈹€鈫?vIP_USARTTask 鈹€鈹€Queue鈹€鈹€鈫?vIP_LEDTask
  鈹?                   瑙ｆ瀽鍛戒护               鈹?LED+OLED
  鈹?                                          鈹?  鈹?KEY涓柇 鈹€鈹€鈹€Sem鈹€鈹€鈫?vIP_KEYTask 鈹€鈹€Queue鈹€鈹€鈫?vIP_W25Q64Task
  鈹?                  娑堟姈+璇嗗埆             鈹?SPI Flash
  鈹?                                        鈹?  鈹?                   vIP_OLEDTask           vIP_PA11Flick
  鈹?                   寮€灞忓姩鐢?              钃濊壊灏忕伅甯搁棯
```

姣忎釜浠诲姟鍙仛涓€浠朵簨锛岄槦鍒楄В鑰︼紝淇敼涓€涓笉褰卞搷鍏朵粬銆?
### 4. 鍏ㄥ眬鐘舵€佸畨鍏ㄧ瓥鐣?
```c
// 鎵€鏈夎鍐?g_state 鐨勫湴鏂圭粺涓€鍔犻攣
osMutexAcquire(General_MutexHandle, osWaitForever);
g_state.led = new_state;
osMutexRelease(General_MutexHandle);
```

> 瑁告満鏃朵唬鐢?`volatile` + 鐘舵€佹満锛孯TOS 鏃朵唬鐢ㄤ簰鏂ラ攣鈥斺€旀湰璐ㄧ浉鍚岋紝褰㈠紡鍗囩骇

### 5. 涓柇鍥炶皟"蹇繘蹇嚭"瑙勮寖锛堟渶缁堢増锛?
```c
void HAL_GPIO_EXTI_Callback(uint16_t pin) {
    if (pin == KEY1 || pin == KEY2)
        osSemaphoreRelease(KEY_BinarySemHandle);  // 浠呮涓€琛岋紝鏃犲欢鏃躲€佹棤鎵撳嵃
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *h) {
    if (h->Instance == USART1) {
        osSemaphoreRelease(USART_CountingSemHandle);  // 浠呮涓€琛?        HAL_UART_Receive_IT(&huart1, &rxData, 1);
    }
}
```

---

## 涓夈€佸績寰?
- 瀛︿範 FreeRTOS 绗笁澶╁氨瀹屾垚浜嗚８鏈虹涓€鍛ㄧ患鍚堥」鐩殑瀹屾暣閲嶆瀯鈥斺€斾粠鐘舵€佹満鍒板浠诲姟绠￠亾锛屾€濈淮杩佺Щ闈炲父椤虹晠
- 浜掓枼閿佽В鍐充簡涓€涓８鏈烘椂浠ｄ粠鏈兂杩囩殑闂锛?*澶氫釜浠诲姟鍚屾椂鎶?OLED 鎬庝箞鍔?*
- 璁℃暟淇″彿閲忔槸涓插彛鎺ユ敹鐨勬纭瓟妗堚€斺€斾簩杩涘埗鍙兘璁板綍"鏉ヨ繃"锛岃鏁拌兘璁颁綇"鏉ヤ簡鍑犳"
- 6 涓换鍔″悇鍙稿叾鑱?+ 闃熷垪瑙ｈ€︼紝杩欎釜鏋舵瀯宸茬粡鎺ヨ繎瀹為檯浜у搧鐨勮璁￠鏍
