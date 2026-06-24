// dwt.h —— DWT 微秒延时（Cortex-M 调试单元）
#ifndef INC_DWT_H_
#define INC_DWT_H_

#include "app_common.h"

#ifdef DWT_MODULE_ENABLED
void DWT_Init(void);              // 使能 DWT 的 CYCCNT 计数器
void DWT_Delay_us(uint32_t us);   // 微秒级延时（阻塞）
#else
#define DWT_Init()
#define DWT_Delay_us(us)
#endif

#endif
