// Controllable_led_task.h —— LED 红灯控制（GPIO）
#ifndef INC_CONTROLLABLE_LED_TASK_H_
#define INC_CONTROLLABLE_LED_TASK_H_

#include "app_common.h"

#ifdef CONTROLLABLE_LED_MODULE_ENABLED
void vIP_LEDTask(void *argument);          // LED 红灯 GPIO 控制
#endif //CONTROLLABLE_LED_MODULE_ENABLED

#endif //INC_CONTROLLABLE_LED_TASK_H_
