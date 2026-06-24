// oled_task.c —— OLED 初始化任务：
//   上电显示 "STM32: OK"，此后由 LED 任务负责刷新 OLED 内容

#include "oled_task.h"
#include "OLED.h"
#include "uart_dma.h"

#ifdef OLED_MODULE_ENABLED

void vIP_OLEDTask(void *argument)
{
    //Init_OLED();
    OLED_Init();
    OLED_Clear();
    OLED_ShowString(0, 0, "STM32: OK", OLED_8X16);//上电默认显示
    OLED_Update();
    for(;;)
    { 
        osDelay(pdMS_TO_TICKS(100));
    }
}

// 发送 I2C 初始化序列，读取状态位确认 OLED 就绪
void Init_OLED(void)
{
    uint8_t commands[] = {0x00,0x8d,0x14,0xaf,0xa5};
    HAL_I2C_Master_Transmit(&hi2c1,0x78,commands,sizeof(commands)/sizeof(commands[0]),HAL_MAX_DELAY);
    uint8_t rcvd;
    HAL_I2C_Master_Receive(&hi2c1,0x78,&rcvd,sizeof(rcvd),HAL_MAX_DELAY);
    if((rcvd & (0x01 << 6)) == 0)
    {
        UART_DMA_Printf("OLED_OK\r\n");        
    }
}

#endif //OLED_MODULE_ENABLED

