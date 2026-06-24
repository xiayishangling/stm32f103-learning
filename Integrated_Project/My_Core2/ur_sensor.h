// ur_sensor.h —— 超声波测距（TIM2 输入捕获 CH1/CH2）
#ifndef INC_UR_SENSOR_H_
#define INC_UR_SENSOR_H_

#include "app_common.h"
#include "tim.h"

#ifdef UR_SENSOR_ENABLED
void  vIP_UR_Task(void *argument);// 运行超声波状态机
#endif

#ifdef UR_SENSOR_ENABLED
float Ultrasonic_Ranging_State_Machine(void);// 状态机（返回距离 m）
#else
#define Ultrasonic_Ranging_State_Machine()
#endif

#endif
