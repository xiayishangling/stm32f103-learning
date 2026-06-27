//Heartbeat_led.c

#include "Heartbeat_led.h"

#ifdef HEARTBEAT_LED_MODULE_ENABLED

#define BLUE_LED_PORT PA11_LED2_GPIO_Port
#define BLUE_LED_PIN PA11_LED2_Pin

// PA11 蓝色 LED 常闪（心跳指示），500ms 翻转一次
void vIP_PA11Flick(void *argument)
{
    for(;;)
    {
        HAL_GPIO_TogglePin(BLUE_LED_PORT,BLUE_LED_PIN);
        osDelay(500);
    }
}

#endif //HEARTBEAT_LED_MODULE_ENABLED
