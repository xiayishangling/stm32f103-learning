//app_common_err.c

#include "app_common_err.h"
#include "uart_dma.h"

//可生成日志，通过队列发送，然后一次性打印执行结果
#ifdef ERR_AUTOMATIC_PRINTING_MODULE_ENABLED //错误自动打印

void ERR_Printf_Task(void *argument)
{
    for(;;)
    {
        if(g_state.ur_distance_m == UR_ERR_WAIT_ECHO_TIMEOUT)
        {
            UART_DMA_Printf("UR_ERR_WAIT_ECHO_TIMEOUT\r\n");
        }
        else if(g_state.ur_distance_m == UR_ERR_OUTRANGE)
        {
            UART_DMA_Printf("UR_ERR_OUTRANGE\r\n");//连续三次测量失败 
        }
        else if(g_state.ps_voltage_v < 0)
        {
            UART_DMA_Printf("PS_ERR_Wait_ADC_JEOC_TIMEOUT\r\n");
        }

        osDelay(pdMS_TO_TICKS(2000));//防止刷屏
    }
}

#endif //ERR_AUTOMATIC_PRINTING_MODULE_ENABLED
