// dwt.c —— DWT 微秒延时实现
//   使能 Cortex-M 调试单元的 CYCCNT 计数器，通过计数值实现微秒级延时
#include "dwt.h"

#ifdef DWT_MODULE_ENABLED

void DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void DWT_Delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000);
    while ((DWT->CYCCNT - start) < ticks);
}

#endif

