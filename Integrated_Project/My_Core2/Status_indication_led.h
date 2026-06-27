// Status_indication_led.h

#ifndef INC_STATUS_INDICATION_LED_H_
#define INC_STATUS_INDICATION_LED_H_

#include "app_common.h"

#ifdef STATUS_INDICATION_LED_MODULE_ENABLED
void vIP_PC13_StartTask(void *argument);   // PC13 指示灯（距离/电压阈值）
#endif //STATUS_INDICATION_LED_MODULE_ENABLED

#endif //INC_STATUS_INDICATION_LED_H_
