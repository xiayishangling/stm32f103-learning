//Heartbeat_led.h

#ifndef INC_HEARTBEAT_LED_H_
#define INC_HEARTBEAT_LED_H_

#include "app_common.h"

#ifdef HEARTBEAT_LED_MODULE_ENABLED
void vIP_PA11Flick(void *argument);        // PA11 心跳闪烁（500ms 周期）
#endif //HEARTBEAT_LED_MODULE_ENABLED

#endif //INC_HEARTBEAT_LED_H_
