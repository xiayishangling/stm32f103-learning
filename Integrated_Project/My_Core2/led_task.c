// led_task.c —— LED 控制 + OLED 显示刷新 + PC13/PA11 辅助任务

#include "led_task.h"
#include "OLED.h"
#include "uart_dma.h"

#ifdef LED_MODULE_ENABLED

#define RED_LED_PORT PA3_LED1_GPIO_Port
#define RED_LED_PIN PA3_LED1_Pin

#define PXXX_LED_PORT PC13_LED3_GPIO_Port
#define PXXX_LED_PIN PC13_LED3_Pin

#define BLUE_LED_PORT PA11_LED2_GPIO_Port
#define BLUE_LED_PIN PA11_LED2_Pin

uint32_t current_time = 0;

// 统一处理 LED 状态和 OLED 显示：
//   - 从 USART_Queue 获取最新 led/oled 状态（来自 CLI 命令或 W25Q64 恢复）
//   - 根据 oled 状态刷新 OLED 屏幕（加互斥锁保护 I2C）
//   - 根据 led 状态控制 PA3（ON/OFF/Flicker）
//   - 传感器页面（电压/距离）每 500ms 强制重绘
void vIP_LEDTask(void *argument)
{
    static uint8_t led_flag = 0;
    OLED_State last_oled_mode = OLED_State_Display3;//上一次状态
    for(;;)
    {
        USARTMessage msg;
        bool new_msg = false;

        if(osMessageQueueGet(USART_QueueHandle,&msg,0,0) == osOK)
        {
            g_state.led = msg.led_state;
            g_state.oled = msg.oled_state;
            new_msg = true;
            led_flag = 0;
        }

        //第一是串口信号 第二是w25q64信号 此处统一改变
        if(new_msg || (g_state.oled != last_oled_mode))
        {
            osMutexAcquire(General_MutexHandle,osWaitForever);
            OLED_Clear();
            switch (g_state.oled)
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
                break;
            case OLED_State_Display_distance:
                break;           
            default:
                OLED_ShowString(0, 0, "STM32: OK", OLED_8X16);//默认显示开机界面
                break;
            }
            OLED_Update();
            osMutexRelease(General_MutexHandle);

            last_oled_mode = g_state.oled;
        }

        switch (g_state.led)
        {
        case LED_State_ON:
            {
                HAL_GPIO_WritePin(RED_LED_PORT,RED_LED_PIN,GPIO_PIN_SET);
                break;
            }
        case LED_State_OFF:
            {
                HAL_GPIO_WritePin(RED_LED_PORT,RED_LED_PIN,GPIO_PIN_RESET);
                break;
            }
        case LED_State_Flicker:
            {
                led_flag = !led_flag;
                HAL_GPIO_WritePin(RED_LED_PORT,RED_LED_PIN,led_flag ? GPIO_PIN_SET : GPIO_PIN_RESET);
                osDelay(pdMS_TO_TICKS(200));
                break;
            }        
        default:
            {
                HAL_GPIO_WritePin(RED_LED_PORT,RED_LED_PIN,GPIO_PIN_RESET);
                break;
            }
        }

        // 传感器页面定时刷新（每 500ms 强制刷新一次）
        static uint32_t sensor_last_update = 0;
        if (g_state.oled == OLED_State_Display_voltage || 
            g_state.oled == OLED_State_Display_distance) 
        {
            if (osKernelGetTickCount() - sensor_last_update >= pdMS_TO_TICKS(500)) 
            {
                sensor_last_update = osKernelGetTickCount();
                // 直接重绘整个屏幕
                osMutexAcquire(General_MutexHandle, osWaitForever);
                OLED_Clear();
                if (g_state.oled == OLED_State_Display_voltage) 
                {
                    OLED_Clear();
                    OLED_ShowString(0, 0, "Voltage:", OLED_8X16);
                    OLED_Printf(0, 16, OLED_8X16, "%.2f V", g_state.ps_voltage_v);
                } 
                else 
                {
                    OLED_Clear();
                    OLED_ShowString(0, 0, "Distance:", OLED_8X16);
                    OLED_Printf(0, 16, OLED_8X16, "%.2f m", g_state.ur_distance_m);
                }
                OLED_Update();
                osMutexRelease(General_MutexHandle);
                last_oled_mode = g_state.oled;  // 避免原有逻辑又触发一次刷新
            }
        }
        
        osDelay(pdMS_TO_TICKS(10));
    }
}

// PC13 指示灯控制：
//   - 距离 < 阈值  → 500ms 闪烁（警示距离过近）
//   - 距离正常     → 根据电压阈值亮灭（电压 > 阈值 灭，否则亮）
void vIP_PC13_StartTask(void *argument)
{
    for(;;)
    {
        if (g_state.ur_distance_m < g_state.distance_threshold) {
            // 距离过近，进入闪烁模式
            static uint32_t flicker_tick = 0;
            if (osKernelGetTickCount() - flicker_tick >= 500) 
            {
                flicker_tick = osKernelGetTickCount();
                HAL_GPIO_TogglePin(PXXX_LED_PORT, PC13_LED3_Pin);
            }
        } else {
            // 正常距离，根据电压亮灭
            if (g_state.ps_voltage_v > g_state.voltage_threshold)
                HAL_GPIO_WritePin(PXXX_LED_PORT, PXXX_LED_PIN, GPIO_PIN_RESET);
            else
                HAL_GPIO_WritePin(PXXX_LED_PORT, PXXX_LED_PIN, GPIO_PIN_SET);
        }
    }
}

// PA11 蓝色 LED 常闪（心跳指示），500ms 翻转一次
void vIP_PA11Flick(void *argument)
{
    for(;;)
    {
        HAL_GPIO_TogglePin(BLUE_LED_PORT,BLUE_LED_PIN);
        osDelay(500);
    }
}

#endif //LED_MODULE_ENABLED

