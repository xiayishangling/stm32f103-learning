// ps_sensor.c —— 光敏传感器（ADC1 注入通道）：
//   状态机 PS_Init → PS_Wait_ADC_JEOC → PS_Process → PS_Idle，2s 一次
//   结果存入 g_state.ps_voltage_v（单位 V，越大越暗）

#include "ps_sensor.h"
#include "uart_dma.h"

#ifdef PS_SENSOR_ENABLED

#define PS_INTERVAL_TIME 2000 //测量间隔 ms
#define PS_TIM_HANDLE htim1
#define PS_ADC_HANDLE hadc1
//static uint32_t ps_currenttick = 0;

//判断注入序列的标志位JEOC 当标志位置1 触发中断 释放PS的二进制信号量 PS状态机会获取此信号量并读数
// ADC1 注入通道转换完成 → 释放信号量通知状态机
void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if(hadc->Instance == ADC1)
    {
        osSemaphoreRelease(PS_BinarySemHandle);
    }
}

//光敏传感器状态机 返回值需 /1000.0f 单位是：V
// 光敏传感器四态状态机，返回值 /1000.0f = 电压（V）
float Photosensitive_Sensor_State_Machine_IS(void)
{
    uint32_t v = 0;
    uint32_t jdr = 0;
    switch (g_state.ps_state)
    {
    case PS_Init:
        {
            HAL_TIM_Base_Start(&PS_TIM_HANDLE);
            g_state.ps_state = PS_Wait_ADC_JEOC;
            break;
        }
    case PS_Wait_ADC_JEOC:
        {
            HAL_ADCEx_InjectedStart_IT(&PS_ADC_HANDLE);
            if(osSemaphoreAcquire(PS_BinarySemHandle,pdMS_TO_TICKS(100)) == osOK)
            {
                g_state.ps_state = PS_Process;
            }
            else
            {
                UART_DMA_Printf("PS_ISR_TIMEOUT\r\n");
                g_state.ps_state = PS_Idle;
            }
            break;
        }
    case PS_Process:
        {
            jdr =  HAL_ADCEx_InjectedGetValue(&PS_ADC_HANDLE,ADC_INJECTED_RANK_1);
            HAL_ADCEx_InjectedStop_IT(&PS_ADC_HANDLE);

            v = (jdr * 3300) / 4095;
            g_state.ps_voltage_v = (v / 1000.0f);
            //UART_DMA_Printf("voltage: %lu.%03lu mv\r\n", v / 1000, v % 1000);
            g_state.ps_state = PS_Idle;
            break;
        }
    case PS_Idle:
        {
                HAL_TIM_Base_Stop(&PS_TIM_HANDLE);
                HAL_ADCEx_InjectedStop_IT(&PS_ADC_HANDLE);
                osDelay(pdMS_TO_TICKS(PS_INTERVAL_TIME));//测量间隔 2s
                g_state.ps_state = PS_Init;
            break;
        }
    }
    return (v / 1000.0f);       
}

void vIP_PS_IJ_Task(void *argument)
{
    for(;;)
    {
        Photosensitive_Sensor_State_Machine_IS();
    }
}

#endif //PS_SENSOR_ENABLED

