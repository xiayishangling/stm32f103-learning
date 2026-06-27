// Controllable_led_task.c —— LED 红灯控制（GPIO）

#include "Controllable_led_task.h"
#include "uart_dma.h"

#ifdef CONTROLLABLE_LED_MODULE_ENABLED

#define RED_LED_PORT PA3_LED1_GPIO_Port
#define RED_LED_PIN PA3_LED1_Pin

// 从 USART_Queue 获取 LED 状态（来自 CLI），控制 PA3（ON/OFF/Flicker）
// OLED 显示由 CLI 命令统一发送到 OLED_Display_QueueHandle
void vIP_LEDTask(void *argument)
{
    static uint8_t led_flag = 0;
    for(;;) 
    {
        USARTMessage led_msg;
        if (osMessageQueueGet(USART_QueueHandle, &led_msg, 0, 0) == osOK) 
        {
            g_state.led = led_msg.led_state;
            led_flag = 0;
        }

        // LED 灯控制
        switch (g_state.led) {
            case LED_State_ON:
                HAL_GPIO_WritePin(RED_LED_PORT, RED_LED_PIN, GPIO_PIN_SET);
                break;
            case LED_State_OFF:
                HAL_GPIO_WritePin(RED_LED_PORT, RED_LED_PIN, GPIO_PIN_RESET);
                break;
            case LED_State_Flicker:
                led_flag = !led_flag;
                HAL_GPIO_WritePin(RED_LED_PORT, RED_LED_PIN, led_flag ? GPIO_PIN_SET : GPIO_PIN_RESET);
                osDelay(pdMS_TO_TICKS(200));
                break;
            default:
                HAL_GPIO_WritePin(RED_LED_PORT, RED_LED_PIN, GPIO_PIN_RESET);
                break;
        }
        osDelay(pdMS_TO_TICKS(10));
    }
}

#endif //CONTROLLABLE_LED_MODULE_ENABLED

