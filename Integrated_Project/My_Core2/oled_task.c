// oled_task.c —— OLED 显示任务：上电显示 "STM32: OK"，由 CLI 命令 / W25Q64 恢复发送显示请求刷新

#include "oled_task.h"
#include "OLED.h"

#ifdef OLED_MODULE_ENABLED

#define OLED_RENOVATE_TIME 1200 //oled强制刷新时间

void vIP_OLEDTask(void *argument)
{
    OLED_Init();
    OLED_Clear();
    OLED_ShowString(0, 0, "STM32: OK", OLED_8X16);//上电默认显示
    OLED_Update();

    OLED_State oled_current_page = OLED_State_Display3;
    bool force = false;
    OLEDDisplayRequest req;
    for(;;)
    { 
        if(osMessageQueueGet(OLED_Display_QueueHandle,&req,0,0) == osOK)
        {
            oled_current_page = req.oled_state;
            force = req.force_refresh;
        }

        //传感器动态页，每隔 1200ms 强制刷新一次
        static uint32_t last_refresh_time = 0;
        bool need_refresh = force;
        if(oled_current_page == OLED_State_Display_voltage || oled_current_page == OLED_State_Display_distance)
        {
            if(osKernelGetTickCount() - last_refresh_time >= pdMS_TO_TICKS(OLED_RENOVATE_TIME))
            {
                need_refresh = true;
                last_refresh_time = osKernelGetTickCount();
            }
        }

        if (need_refresh || force) 
        {
            osMutexAcquire(General_MutexHandle, osWaitForever);
            OLED_Clear();
            switch (oled_current_page) 
            {
                case OLED_State_Display1:
                    OLED_ShowString(0, 0, "LED: ON", OLED_8X16);
                    break;
                case OLED_State_Display2:
                    OLED_ShowString(0, 0, "LED: OFF", OLED_8X16);
                    break;
                case OLED_State_Display3:
                    OLED_ShowString(0, 0, "STM32: OK", OLED_8X16);
                    break;
                case OLED_State_Display_voltage:
                case OLED_State_Display_distance:
                    {
                        Sensor *s = req.Sensor;
                        if(s)
                        {
                            float val = s->get_value(s);
                            if(val > 0)
                            {
                                OLED_ShowString(0,0,s->display_name,OLED_8X16);
                                OLED_Printf(0,16,OLED_8X16,"%.3f %s",val,s->get_uint(s));
                            }
                            else
                            {
                                OLED_ShowString(0, 0, "Invalid", OLED_8X16);
                            }
                        }
                        break;
                    }
                default:
                    OLED_ShowString(0, 0, "STM32: OK", OLED_8X16);
                    break;
            }
            OLED_Update();
            osMutexRelease(General_MutexHandle);
            force = false;
        }
        osDelay(pdMS_TO_TICKS(50));
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
        //UART_DMA_Printf("OLED_OK\r\n");        
    }
}

#endif //OLED_MODULE_ENABLED

