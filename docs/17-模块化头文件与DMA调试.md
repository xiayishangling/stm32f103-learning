# 6.22 蹇冨緱 鈥斺€?妯″潡鍖栧ご鏂囦欢 & My_Code3

> 鎸佺画鏇存柊 | 鏈€鍚庢洿鏂帮細2026-06-22

---

## 涓€銆佸ご鏂囦欢妯″潡鍖栧紑鍏虫ā寮?
My_Code3.h 灞曠ず浜嗘渶瀹屾暣鐨勬潯浠剁紪璇戞ā寮忥細

```c
#ifdef MODULE_ENABLED
    // 鐪熷疄鍑芥暟澹版槑銆佸彉閲忓０鏄?    void Func(void);
    extern uint8_t buffer[64];
#else
    // 绌哄畯锛岃皟鐢ㄨ鏇挎崲涓虹┖
    #define Func()
#endif
```

**鏍稿績鎬濇兂**锛氶€氳繃 `app_config.h` 涓殑瀹忓紑鍏筹紝涓€琛屾敞閲婂氨鑳藉惎鐢?绂佺敤涓€涓ā鍧楋紝鏃犻渶鏀瑰姩涓氬姟浠ｇ爜銆?
### 鍚勬ā鍧楀姣?
| 鏂囦欢 | 鍚敤寮€鍏?| 妯″紡 |
|------|----------|------|
| My_Code1.h | `SIX_MODULE_LINGKAGE_FREERTOS_ENABLED` | 鍗曞眰 `#ifdef`锛堟棤 `#else` 绌哄畯锛?|
| My_Code2.h | `KEY_DEBOUNCE_MODULE_ENABLED` | 鍗曞眰 `#ifdef` |
| My_Code3.h | `HAL_UART_MODULE_ENABLED` / `TEST_CPU_USAGE_ENABLED` | **鍙屽眰鏉′欢缂栬瘧** + `#else` 绌哄畯 |

### 澶存枃浠跺叕鍏变緷璧栭摼

```
My_CodeX.h
  鈹斺攢鈹€ app_common.h        (鍏叡缁撴瀯浣撱€佹灇涓俱€佸閮ㄥ０鏄?
        鈹溾攢鈹€ stm32f1xx_hal.h
        鈹溾攢鈹€ cmsis_os.h
        鈹溾攢鈹€ main.h
        鈹溾攢鈹€ <stdbool.h>
        鈹溾攢鈹€ <string.h>
        鈹溾攢鈹€ <stdio.h>
        鈹溾攢鈹€ <stdlib.h>
        鈹溾攢鈹€ <math.h>
        鈹斺攢鈹€ app_config.h   (妯″潡寮€鍏?
```

鎵€鏈?My_CodeX.h 鍙渶 `#include "app_common.h"`锛屾棤闇€閲嶅寮曞叆 STM32 澶存枃浠躲€?
---

## 浜屻€丮y_Code3 鏍稿績鍐呭

### 2.1 UART DMA 鏀跺彂

```c
// 澶存枃浠跺畯瀹氫箟
#define ADD_DMA_USART       huart1          // 缁戝畾鍏蜂綋澶栬瀹炰緥
#define TX_DMA_BUFFER_SIZE  64
#define RX_DMA_BURRER_SIZE  64

// DMA 鎵撳嵃瀹忥紙闈為樆濉?printf锛?#define UART_DMA_Printf(...) do { \
    int len = sprintf((char *)tx_dma_buffer, __VA_ARGS__); \
    HAL_UART_Transmit_DMA(&ADD_DMA_USART, tx_dma_buffer, len); \
} while(0)
```

**鍏抽敭鐐?*锛?- `HAL_UARTEx_ReceiveToIdle_DMA` 瀹炵幇涓嶅畾闀挎帴鏀讹紙绌洪棽涓柇瑙﹀彂鍥炶皟锛?- `UART_DMA_Printf` 鐢?DMA 浼犺緭浠ｆ浛闃诲鎵撳嵃锛屼笉鍗?FreeRTOS 璋冨害
- 浣跨敤 `do { ... } while(0)` 鍖呰９瀹忥紝纭繚鍦ㄤ换浣?`if/else` 涓婁笅鏂囦腑瀹夊叏灞曞紑

### 2.2 CPU 浣跨敤鐜囩洃鎺т换鍔?
```c
void T_CPU_U_Task(void *argument)
{
    HAL_TIM_Base_Start_IT(&htim3);
    for(;;)
    {
        if(tim3_finish_flag)
        {
            uint32_t start = __HAL_TIM_GetCounter(&htim3);
            tim3_finish_flag = 0;
            // ... 涓氬姟澶勭悊 ...
            cpu_use = (float)((__HAL_TIM_GetCounter(&htim3) - start) / 5000.f) * 100;
        }
    }
}
```

**鍘熺悊**锛歍IM3 瀹氭椂涓柇 鈫?缃爣蹇椾綅 鈫?浠诲姟娴嬮噺绌洪棽璁℃暟宸€?鈫?鍙嶇畻 CPU 鍗犵敤鐜囥€?
---

## 涓夈€佸瘎瀛樺櫒浣嶆搷浣滃畯

```c
#ifndef SET_BIT
#define SET_BIT(REG, BIT)    ((REG) |= (BIT))     // 鍐?|=   (缃?)
#endif

#ifndef CLEAR_BIT
#define CLEAR_BIT(REG, BIT)  ((REG) &= (~BIT))    // 娓呴櫎 &=~ (娓?)
#endif

#ifndef READ_BIT
#define READ_BIT(REG, BIT)   ((REG) & (BIT))      // 璇诲彇 &   (鍒ゆ柇)
#endif

// 绀轰緥锛氱偣浜?PC13 鐨?LED
SET_BIT(GPIOC->ODR, GPIO_PIN_13);
// 绛変环浜?GPIOC->ODR |= 0x2000; (绗?3浣嶅彉1锛屽叾浣欎笉鍙?
```

---

## 鍥涖€丗reeRTOS 鍫嗗唴瀛樺竷灞€

`configTOTAL_HEAP_SIZE = 10240` 涓嶄細鍜岀郴缁?Heap/Stack 鍐茬獊锛?
```
SRAM 甯冨眬锛堢ず鎰忥級锛?鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹? 绯荤粺 Stack (0x400)  鈹? 鈫?鐢卞惎鍔ㄦ枃浠跺垎閰?鈹溾攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹? 绯荤粺 Heap  (0x200)  鈹? 鈫?malloc 浠庤繖閲屾嬁
鈹溾攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹? FreeRTOS ucHeap[]   鈹? 鈫?heap_4.c 涓?static uint8_t ucHeap[10240]
鈹? (10KB)                鈹?   鐙珛浜庣郴缁熷爢鏍堬紝浜掍笉骞叉壈
鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?```

FreeRTOS heap_4 鐢?`static uint8_t ucHeap[]` 鐙珛鍒掍簡涓€鍧?`.bss` 娈碉紝鏃笉鍗犵敤绯荤粺 Heap 涔熶笉鍗犵敤绯荤粺 Stack銆?
---

## 浜斻€佷粖鏃ュ彂鐜扮殑闂

| 闂 | 浣嶇疆 | 璇存槑 |
|------|------|------|
| `RX_DMA_BURRER_SIZE` | My_Code3.h/c | 鎷煎啓閿欒锛屽簲涓?`BUFFER` |
| `TEST_CPU_USAGE_ENABLE` | app_config.h | 灏戜簡涓?`D`锛屽簲涓?`ENABLED` |

杩欎簺鎷煎啓涓嶄竴鑷磋櫧鐒朵笉褰卞搷缂栬瘧锛堝洜涓哄紩鐢ㄥ淇濇寔涓€鑷达級锛屼絾寤鸿缁熶竴淇銆?
