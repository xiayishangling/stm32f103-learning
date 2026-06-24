// oled_task.h —— OLED 初始化（I2C）
#ifndef INC_OLED_TASK_H_
#define INC_OLED_TASK_H_

#include "app_common.h"

#ifdef OLED_MODULE_ENABLED
void vIP_OLEDTask(void *argument);// OLED 任务：上电显示 "STM32: OK"
void Init_OLED(void);// I2C 初始化序列 + 状态检测
#endif

#endif
