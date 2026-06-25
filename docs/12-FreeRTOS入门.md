# 6.15 FreeRTOS 鍏ラ棬 鈥?棣栦釜澶氫换鍔?+ 鏋舵瀯鍏ㄦ櫙

> **鑺墖**锛歋TM32F103C8T6 | **鐜**锛欳ubeMX + VSCode + Keil  
> **涓婚**锛欶reeRTOS 宸ョ▼鎼缓銆佷笁浠诲姟璋冨害銆佸唴鏍告灦鏋勩€丆 璇█鍐呭瓨/浣滅敤鍩熷己鍖?
---

## 涓€銆佹牳蹇冧唬鐮?
### 1. C 璇█寮哄寲锛氶潤鎬佸眬閮ㄥ彉閲?+ 鍔ㄦ€佸唴瀛?
```c
// 闈欐€佸眬閮ㄥ彉閲忥細鍑芥暟閫€鍑轰笉閿€姣侊紝鍊间繚鐣?static int static_globalvar = 100;  // 闈欐€佸叏灞€鍙橀噺 鈥?浠呮湰鏂囦欢鍙

int getnextsquare()
{
    static int num = 1;     // 闈欐€佸眬閮細鐢熷懡鍛ㄦ湡 = 鏁翠釜绋嬪簭
    int ret = num * num;
    num++;
    return ret;
}

// 涓诲嚱鏁帮細鍔ㄦ€佸垎閰?n 涓钩鏂规暟
int main(int argc, char *argv[])
{
    int n;
    scanf_s("%d", &n);
    int *pbuff = (int *)malloc(sizeof(int) * n);  // 鍫嗗彉閲?    for (int i = 0; i < n; i++)
        pbuff[i] = getnextsquare();
    // ... 鎵撳嵃 ...
    free(pbuff);  // 濂戒範鎯細閲婃斁鍫嗗唴瀛?    return 0;
}
```

> 鍙橀噺鍥涜薄闄愶細鍏ㄥ眬 鈫?闈欐€佸叏灞€ 鈫?闈欐€佸眬閮?鈫?鍫?malloc) 鈫?灞€閮?鏍?

### 2. FreeRTOS 棣栦釜澶氫换鍔?
```c
#include "FreeRTOS.h"
#include "task.h"

// 浠诲姟1锛歀ED1 100ms 闂儊
void vLED1Task(void *argument)
{
    while (1) {
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
        vTaskDelay(pdMS_TO_TICKS(100));
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// 浠诲姟2锛歀ED2 30ms/150ms 闈炲绉伴棯鐑?void vLED2Task(void *argument)
{
    while (1) {
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
        vTaskDelay(pdMS_TO_TICKS(30));
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
        vTaskDelay(pdMS_TO_TICKS(150));
    }
}

// 浠诲姟3锛氫覆鍙ｆ瘡 1s 鍙戦€?"浣犲ソ涓栫晫"
void usart_send_helloworld(void *task)
{
    while (1) {
        HAL_UART_Transmit(&huart1, (uint8_t*)"浣犲ソ涓栫晫\n", strlen(...), HAL_MAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// 鍚姩
xTaskCreate(vLED1Task, "LED1", 128, NULL, 1, NULL);
xTaskCreate(vLED2Task, "LED2", 128, NULL, 1, NULL);
xTaskCreate(usart_send_helloworld, "UART", 128, NULL, 1, NULL);
vTaskStartScheduler();  // 璋冨害鍣ㄥ惎鍔紝姘镐笉杩斿洖
```

---

## 浜屻€佹牳蹇冪煡璇嗙偣

### 1. FreeRTOS 涓夊ぇ纭欢鍩虹煶

| 寮傚父 | 浣滅敤 | 浼樺厛绾?| 鍙ｈ瘈 |
|------|------|--------|------|
| **SysTick** | 鎻愪緵 tick 鏃堕挓锛岄┍鍔ㄦ椂闂寸墖 | 鏈€浣?| 鎶ユ椂 |
| **SVC** | 鍚姩绗竴涓换鍔?| **鏈€楂?* | 璧疯窇 |
| **PendSV** | 鎵ц涓婁笅鏂囧垏鎹?| 鏈€浣?| 鎹㈠満涓嶆墦鎵?|

> PendSV 浼樺厛绾ф渶浣庯紝淇濊瘉鎵€鏈?ISR 鎵ц瀹屾瘯鍐嶅垏鎹⑩€斺€旇繖鏄?RTOS 璁捐鐨勭簿楂?
### 2. 璋冨害鍣ㄤ笁绉嶆ā寮?
| 妯″紡 | 琛屼负 |
|------|------|
| 甯︽椂闂寸墖鎶㈠崰 | 鍚屼紭鍏堢骇杞祦鎵ц锛坄configUSE_TIME_SLICING=1`锛?|
| 涓嶅甫鏃堕棿鐗囨姠鍗?| 楂樹紭鍏堢骇绔嬪嵆鎶㈠崰锛屽悓绾ч渶涓诲姩璁╁嚭 CPU |
| 鍗忎綔寮?| 浠诲姟蹇呴』涓诲姩閲婃斁锛屽惁鍒欑嫭鍗?|

### 3. xTaskCreate 鍏弬鏁?
```
xTaskCreate(浠诲姟鍑芥暟, "鍚嶇О", 鏍堟繁搴?瀛?, 鍙傛暟, 浼樺厛绾? 鍙ユ焺)
```

| 鍙傛暟 | 璇存槑 | 甯歌鍧?|
|------|------|--------|
| 鏍堟繁搴?| 鍗曚綅 word(4瀛楄妭) | 璁惧お灏?鈫?鏍堟孩鍑猴紝寤鸿 脳1.5~2 鍊嶄及绠?|
| 浼樺厛绾?| 鏁板€艰秺澶ц秺楂?| 0 鏄┖闂蹭换鍔′紭鍏堢骇 |

### 4. 浜旂鍫嗘柟妗堬紙heap_1~5锛?
| 鏂规 | 鐗圭偣 | 閫傜敤 |
|------|------|------|
| heap_1 | 鍙垎閰嶄笉閲婃斁 | 闈欐€佷换鍔?|
| heap_2 | 鍙噴鏀撅紝涓嶅悎骞剁鐗?| 宸茶繃鏃?|
| heap_3 | 灏佽 malloc/free | 闇€淇濊瘉绾跨▼瀹夊叏 |
| **heap_4** | **鍚堝苟绌洪棽鍧?* | **鏈€甯哥敤** |
| heap_5 | heap_4 + 璺ㄥ鍧楀唴瀛?| 鏈夊閮?SRAM 鏃?|

### 5. FreeRTOSConfig.h 鍏抽敭閰嶇疆

```c
#define configCPU_CLOCK_HZ                    72000000   // 涓婚
#define configTICK_TYPE_WIDTH_IN_BITS         TICK_TYPE_WIDTH_32_BITS
#define configKERNEL_INTERRUPT_PRIORITY       (15 << 4)  // ST鍙敤楂?浣?#define configMAX_SYSCALL_INTERRUPT_PRIORITY  (5 << 4)   // 楂樹簬姝や笉鍙桝PI骞叉壈
#define configUSE_TIME_SLICING                1
#define configCHECK_FOR_STACK_OVERFLOW        0           // 璋冭瘯闃舵鏆傛椂鍏抽棴
```

### 6. 浠诲姟闂撮€氫俊鍏ㄥ绂?
```
闃熷垪 鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€ 鏁版嵁浼犺緭锛堝揩閫掍紶閫佸甫锛?浜岃繘鍒朵俊鍙烽噺 鈹€鈹€鈹€鈹€ 鍚屾锛堢孩缁跨伅锛?璁℃暟淇″彿閲?鈹€鈹€鈹€鈹€鈹€鈹€ 璧勬簮绠＄悊锛堝仠杞︿綅闂告満锛?浜掓枼閲?鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€ 浼樺厛绾у弽杞繚鎶?浜嬩欢缁?鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€ 澶氭潯浠朵笌/鎴栬Е鍙?浠诲姟閫氱煡 鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€ 杞婚噺绾у崟鍊奸偖绠?```

### 7. 鍙橀噺鍥涜薄闄愶紙闈㈣瘯楂橀锛?
| 绫诲瀷 | 浣滅敤鍩?| 鐢熷懡鍛ㄦ湡 | 瀛樺偍鍖?|
|------|--------|----------|--------|
| 鍏ㄥ眬鍙橀噺 | 鎵€鏈夋枃浠?| 鏁翠釜绋嬪簭 | 鏁版嵁娈?|
| 闈欐€佸叏灞€ | 褰撳墠鏂囦欢 | 鏁翠釜绋嬪簭 | 鏁版嵁娈?|
| **闈欐€佸眬閮?* | 褰撳墠鍑芥暟 | **鏁翠釜绋嬪簭** | 鏁版嵁娈?|
| 鍫嗗彉閲?| 鏈夋寚閽堝 | free 涓烘 | 鍫?|
| 灞€閮ㄥ彉閲?| 褰撳墠鍑芥暟 | 鍑芥暟閫€鍑?| 鏍?|

> 闈欐€佸眬閮ㄥ彉閲忎笉闅忓嚱鏁板帇鏍堝嚭鏍堚€斺€旂紪璇戞椂鍒嗛厤鍦ㄦ暟鎹锛屽浐瀹氬湴鍧€

---

## 涓夈€佸績寰?
- 瑁告満鐘舵€佹満 鈫?FreeRTOS 浠诲姟锛屼笉鏄帹鍊掗噸鏉ワ紝鏄妸 `switch-case` 鍗囩骇涓虹郴缁熷府浣犺皟搴?- SysTick/SVC/PendSV 涓変釜寮傚父鏄?RTOS 鐨勫叏閮ㄧ‖浠朵緷璧栤€斺€旂悊瑙ｅ畠浠氨鐞嗚В浜?鎿嶄綔绯荤粺"鐨勬湰璐?- 鏍堟繁搴?128 鏄粡楠岃捣鐐癸紝姝ｅ紡椤圭洰鐢?`uxTaskGetStackHighWaterMark()` 瀹炴祴鍚庡啀璋冧紭
