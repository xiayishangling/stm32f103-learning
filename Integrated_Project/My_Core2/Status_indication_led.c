//Status_indication_led.c

#include "Status_indication_led.h"

#ifdef STATUS_INDICATION_LED_MODULE_ENABLED

#define PXXX_LED_PORT PC13_LED3_GPIO_Port
#define PXXX_LED_PIN PC13_LED3_Pin

// PC13 指示灯控制：
//   - 距离 < 阈值  → 500ms 闪烁（警示距离过近）
//   - 距离正常     → 根据电压阈值亮灭（电压 > 阈值 灭，否则亮）
void vIP_PC13_StartTask(void *argument)
{
    for(;;)
    {
        SensorNotifyMsg msg;
        osMessageQueueGet(Sensor_Notify_QueueHandle,&msg,0,osWaitForever);

        if (g_state.ur_distance_m < g_state.distance_threshold && g_state.ur_distance_m > 0)
        {
            // 距离过近，进入闪烁模式
            static uint32_t flicker_tick = 0;
            if (osKernelGetTickCount() - flicker_tick >= 500) 
            {
                flicker_tick = osKernelGetTickCount();
                HAL_GPIO_TogglePin(PXXX_LED_PORT, PC13_LED3_Pin);
            }
        } 
        else if(g_state.ps_voltage_v > 0)
        {
            // 正常距离，根据电压亮灭
            if (g_state.ps_voltage_v > g_state.voltage_threshold)
                HAL_GPIO_WritePin(PXXX_LED_PORT, PXXX_LED_PIN, GPIO_PIN_RESET);
            else
                HAL_GPIO_WritePin(PXXX_LED_PORT, PXXX_LED_PIN, GPIO_PIN_SET);
        }
    }
}

#endif //STATUS_INDICATION_LED_MODULE_ENABLED
