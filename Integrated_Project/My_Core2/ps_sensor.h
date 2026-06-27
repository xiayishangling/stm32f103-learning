// ps_sensor.h —— 光敏传感器（ADC1 注入通道 + TIM1 TRGO）
#ifndef INC_PS_SENSOR_H_
#define INC_PS_SENSOR_H_

#include "app_common.h"
#include "adc.h"
#include "tim.h"

#ifdef PS_SENSOR_ENABLED
void vIP_PS_IJ_Task(void *argument);// 运行光敏状态机
float Photosensitive_Sensor_State_Machine_IS(void);// 状态机（返回值 /1000 = V）
#else
#define Photosensitive_Sensor_State_Machine_IS()
#endif //PS_SENSOR_ENABLED

extern Sensor PS_Sensor;

#endif //INC_PS_SENSOR_H_
