// led_task.h —— LED 控制 + OLED 刷新 + 指示灯任务
#ifndef INC_LED_TASK_H_
#define INC_LED_TASK_H_

#include "app_common.h"

#ifdef LED_MODULE_ENABLED
void vIP_LEDTask(void *argument);          // LED 状态控制 + OLED 页面刷新
void vIP_PC13_StartTask(void *argument);   // PC13 指示灯（距离/电压阈值）
void vIP_PA11Flick(void *argument);        // PA11 心跳闪烁（500ms 周期）
#endif

#endif
