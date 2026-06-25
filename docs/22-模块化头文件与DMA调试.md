# 6.22 心得 —— 模块化头文件 & My_Code3

> 持续更新 | 最后更新：2026-06-22

---

## 一、头文件模块化开关模式

My_Code3.h 展示了最完整的条件编译模式：

```c
#ifdef MODULE_ENABLED
    // 真实函数声明、变量声明
    void Func(void);
    extern uint8_t buffer[64];
#else
    // 空宏，调用被替换为空
    #define Func()
#endif
```

**核心思想**：通过 `app_config.h` 中的宏开关，一行注释就能启用/禁用一个模块，无需改动业务代码。

### 各模块对比

| 文件 | 启用开关 | 模式 |
|------|----------|------|
| My_Code1.h | `SIX_MODULE_LINGKAGE_FREERTOS_ENABLED` | 单层 `#ifdef`（无 `#else` 空宏） |
| My_Code2.h | `KEY_DEBOUNCE_MODULE_ENABLED` | 单层 `#ifdef` |
| My_Code3.h | `HAL_UART_MODULE_ENABLED` / `TEST_CPU_USAGE_ENABLED` | **双层条件编译** + `#else` 空宏 |

### 头文件公共依赖链

```
My_CodeX.h
  └── app_common.h        (公共结构体、枚举、外部声明)
        ├── stm32f1xx_hal.h
        ├── cmsis_os.h
        ├── main.h
        ├── <stdbool.h>
        ├── <string.h>
        ├── <stdio.h>
        ├── <stdlib.h>
        ├── <math.h>
        └── app_config.h   (模块开关)
```

所有 My_CodeX.h 只需 `#include "app_common.h"`，无需重复引入 STM32 头文件。

---

## 二、My_Code3 核心内容

### 2.1 UART DMA 收发

```c
// 头文件宏定义
#define ADD_DMA_USART       huart1          // 绑定具体外设实例
#define TX_DMA_BUFFER_SIZE  64
#define RX_DMA_BURRER_SIZE  64

// DMA 打印宏（非阻塞 printf）
#define UART_DMA_Printf(...) do { \
    int len = sprintf((char *)tx_dma_buffer, __VA_ARGS__); \
    HAL_UART_Transmit_DMA(&ADD_DMA_USART, tx_dma_buffer, len); \
} while(0)
```

**关键点**：
- `HAL_UARTEx_ReceiveToIdle_DMA` 实现不定长接收（空闲中断触发回调）
- `UART_DMA_Printf` 用 DMA 传输代替阻塞打印，不卡 FreeRTOS 调度
- 使用 `do { ... } while(0)` 包裹宏，确保在任何 `if/else` 上下文中安全展开

### 2.2 CPU 使用率监控任务

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
            // ... 业务处理 ...
            cpu_use = (float)((__HAL_TIM_GetCounter(&htim3) - start) / 5000.f) * 100;
        }
    }
}
```

**原理**：TIM3 定时中断 → 置标志位 → 任务测量空闲计数差值 → 反算 CPU 占用率。

---

## 三、寄存器位操作宏

```c
#ifndef SET_BIT
#define SET_BIT(REG, BIT)    ((REG) |= (BIT))     // 写 |=   (置1)
#endif

#ifndef CLEAR_BIT
#define CLEAR_BIT(REG, BIT)  ((REG) &= (~BIT))    // 清除 &=~ (清0)
#endif

#ifndef READ_BIT
#define READ_BIT(REG, BIT)   ((REG) & (BIT))      // 读取 &   (判断)
#endif

// 示例：点亮 PC13 的 LED
SET_BIT(GPIOC->ODR, GPIO_PIN_13);
// 等价于 GPIOC->ODR |= 0x2000; (第13位变1，其余不变)
```

---

## 四、FreeRTOS 堆内存布局

`configTOTAL_HEAP_SIZE = 10240` 不会和系统 Heap/Stack 冲突：

```
SRAM 布局（示意）：
┌──────────────────────┐
│  系统 Stack (0x400)  │  ← 由启动文件分配
├──────────────────────┤
│  系统 Heap  (0x200)  │  ← malloc 从这里拿
├──────────────────────┤
│  FreeRTOS ucHeap[]   │  ← heap_4.c 中 static uint8_t ucHeap[10240]
│  (10KB)                │    独立于系统堆栈，互不干扰
└──────────────────────┘
```

FreeRTOS heap_4 用 `static uint8_t ucHeap[]` 独立划了一块 `.bss` 段，既不占用系统 Heap 也不占用系统 Stack。

---

## 五、今日发现的问题

| 问题 | 位置 | 说明 |
|------|------|------|
| `RX_DMA_BURRER_SIZE` | My_Code3.h/c | 拼写错误，应为 `BUFFER` |
| `TEST_CPU_USAGE_ENABLE` | app_config.h | 少了个 `D`，应为 `ENABLED` |

这些拼写不一致虽然不影响编译（因为引用处保持一致），但建议统一修正。
