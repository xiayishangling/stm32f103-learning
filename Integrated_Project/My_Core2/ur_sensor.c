// ur_sensor.c —— 超声波测距（TIM2 输入捕获）：
//   状态机 UR_Send_Pulse → UR_Wait_Echo → UR_Process → UR_Idle，2s 一次
//   CC1 捕获上升沿 / CC2 捕获下降沿 → 脉宽计算距离 → 存入 g_state.ur_distance_m

#include "ur_sensor.h"
#include "dwt.h"
#include "uart_dma.h"

#ifdef UR_SENSOR_ENABLED

#define UR_TIMEOUT_TIME 30 //ms
#define UR_INTERVAL_TIME 2000 //ms
#define UR_SEND_PULSE_GPIO_PORT PB14_TIM2_UR_GPIO_Port
#define UR_SEND_PULSE_GPIO_PIN PB14_TIM2_UR_Pin
#define UR_TIM_HANDLE htim2

static uint8_t capture_flag = 0;
//static uint32_t ur_currentTick = 0;

//超声波CC1 CC2 中断 检查标志位
// TIM2 CH1/CH2 输入捕获中断 → 两个通道都触发后释放信号量
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM2)
    {
        if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
        {
            capture_flag |= UR_FLAG_CH1_CC1;
        }
        else if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
        {
            capture_flag |= UR_FLAG_CH2_CC2;
        }

        //检查两个标志位是否都触发
        if((capture_flag & (UR_FLAG_CH1_CC1 | UR_FLAG_CH2_CC2)) == (UR_FLAG_CH1_CC1 | UR_FLAG_CH2_CC2))
        {
            osSemaphoreRelease(UR_BinarySemHandle);
            capture_flag = 0;
        }
    }
}

//超声波测距状态机 返回值是距离 单位：m
// 超声波四态状态机，返回距离（m）
// 发送 10us 脉冲 → 等待回声 → 计算 (高电平脉宽 * 声速) / 2
float Ultrasonic_Ranging_State_Machine(void)
{
    uint32_t ccr1 = 0;
    uint32_t ccr2 = 0;
    float distance = 0;
    switch (g_state.ur_state)
    {
    case UR_Send_Pulse_to_Trig:
        {		
            //1.清空CNT
            __HAL_TIM_SET_COUNTER(&UR_TIM_HANDLE,0);
            //2.清空cc1 cc2
            __HAL_TIM_CLEAR_FLAG(&UR_TIM_HANDLE,TIM_FLAG_CC1);
            __HAL_TIM_CLEAR_FLAG(&UR_TIM_HANDLE,TIM_FLAG_CC2);
            capture_flag = 0;//每次发送清空中断标志位 

            HAL_TIM_IC_Start_IT(&UR_TIM_HANDLE,TIM_CHANNEL_1);
            HAL_TIM_IC_Start_IT(&UR_TIM_HANDLE,TIM_CHANNEL_2);

            //3.发送10us的脉冲
            HAL_GPIO_WritePin(UR_SEND_PULSE_GPIO_PORT,UR_SEND_PULSE_GPIO_PIN,GPIO_PIN_SET);
            DWT_Delay_us(10);
            HAL_GPIO_WritePin(UR_SEND_PULSE_GPIO_PORT,UR_SEND_PULSE_GPIO_PIN,GPIO_PIN_RESET);

            g_state.ur_state = UR_Wait_Echo;
            break;
        }

    case UR_Wait_Echo:
        {
            if(osSemaphoreAcquire(UR_BinarySemHandle,pdMS_TO_TICKS(100)) == osOK)
            {
                g_state.ur_state = UR_Process;
                break;
            }
            else
            {
                UART_DMA_Printf("UR_TimeOUt\r\n");
                g_state.ur_state = UR_Idle;
            }
            break;
        }	

    case UR_Process:
        {
            ccr1 = __HAL_TIM_GET_COMPARE(&UR_TIM_HANDLE,TIM_CHANNEL_1);
            ccr2 = __HAL_TIM_GET_COMPARE(&UR_TIM_HANDLE,TIM_CHANNEL_2);
            uint32_t width = (ccr2 >= ccr1) ? (ccr2 - ccr1) : ((0xFFFF - ccr1) + ccr2 +1);//us
            if(width > 100 && width < 20000)//有效距离约为： 1.7 cm ~ 3.4 m
            {
                distance = (width*1.0e-6f*340.0f)/2.0f;
                g_state.ur_distance_m = distance;
                //UART_DMA_Printf("distance: %.3f m\r\n",distance);
            }
            g_state.ur_state = UR_Idle;
            break;
        }
    
    case UR_Idle:
        {
            HAL_TIM_IC_Stop_IT(&UR_TIM_HANDLE,TIM_CHANNEL_1);
            HAL_TIM_IC_Stop_IT(&UR_TIM_HANDLE,TIM_CHANNEL_2);
            osDelay(pdMS_TO_TICKS(UR_INTERVAL_TIME));//测量间隔 2s
            g_state.ur_state = UR_Send_Pulse_to_Trig;                
            break;
        }
    }
    return distance;
}

void vIP_UR_Task(void *argument)
{
    for(;;)
    {
        Ultrasonic_Ranging_State_Machine();
    }
}

#endif //UR_SENSOR_ENABLED

