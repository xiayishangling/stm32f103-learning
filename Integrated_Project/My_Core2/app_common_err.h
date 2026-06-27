// app_common_err.h 公共错误返回值

#ifndef INC_APP_COMMON_ERR_H_
#define INC_APP_COMMON_ERR_H_

#include "app_common.h"

//超声波模块
#define UR_ERR_WAIT_ECHO_TIMEOUT -1.0f  //等待echo超时 错误
#define UR_ERR_OUTRANGE -2.0f  //距离不符合范围 错误

//光敏传感器模块
#define PS_ERR_Wait_ADC_JEOC_TIMEOUT -1.0f

#ifdef ERR_AUTOMATIC_PRINTING_MODULE_ENABLED //错误自动打印
void ERR_Printf_Task(void *argument);
#endif //ERR_AUTOMATIC_PRINTING_MODULE_ENABLED

#endif //INC_APP_COMMON_ERR_H_
