// app_common.h 公共结构体 头文件
#ifndef INC_APP_COMMON_H_
#define INC_APP_COMMON_H_

#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "main.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "app_config.h"
#include "sensor_interface.h"

#ifdef APP_COMMON_ENABLED

extern UART_HandleTypeDef huart1;//串口1句柄
extern I2C_HandleTypeDef hi2c1;//I2C句柄
extern SPI_HandleTypeDef hspi1;//SPI句柄
extern ADC_HandleTypeDef hadc1;//ADC1句柄
extern TIM_HandleTypeDef htim1;//定时器1句柄  光敏传感器
extern TIM_HandleTypeDef htim2;//定时器2句柄  超声波测距
extern TIM_HandleTypeDef htim3;//定时器3句柄  测量cpu使用率
extern osSemaphoreId_t USART_CountingSemHandle;//串口计数信号量
extern osSemaphoreId_t KEY_BinarySemHandle;//按键二进制信号量 通知按键已经按下
extern osSemaphoreId_t PS_BinarySemHandle;//光敏传感器模块使用的二进制信号量
extern osSemaphoreId_t UR_BinarySemHandle;//超声波模块使用的二进制信号量
extern osMutexId_t General_MutexHandle;//通用互斥锁
extern osMessageQueueId_t KEY_QueueHandle;//按键队列 传递W25Q64状态
extern osMessageQueueId_t USART_QueueHandle;//串口队列 传递LED状态
extern osMessageQueueId_t OLED_Display_QueueHandle;//OLED队列，刷新OLED状态
extern osMessageQueueId_t Sensor_Notify_QueueHandle;//取代Status_indication_led的轮询，改为通知

//——————————枚举区 开始——————————
//led状态枚举
typedef enum{
    LED_State_ON,
    LED_State_OFF,
    LED_State_Flicker
}LED_State;

//w25q64状态枚举
typedef enum{
    W25Q64_State_Save,
    W25Q64_State_Recovery
}W25Q64_State;

//oled状态枚举
typedef enum{
    OLED_State_Display1,//显示 LED:ON
    OLED_State_Display2,//显示 LED:OFF
    OLED_State_Display3,//显示 STM32:0K
    OLED_State_Display_voltage,//显示光敏传感器的电压
    OLED_State_Display_distance//显示超声波测距距离
}OLED_State;

//传感器状态枚举
typedef enum{
    SENSOR_PS_UPDATAED,//更新 光敏传感器方
    SENSOR_UR_UPDATAED,//更新 超声波模块方
}SensorEvent;
//——————————枚举区 结束——————————


//——————————位掩码 宏 开始————————————
//位索引枚举
typedef enum{
    UR_BIT_CH1 = 0,
    UR_BIT_CH2 = 1,
    //...
}Bit_index_enum;

//位掩码 宏
#define BIT_MASK_MACRO(bit) (1U << (bit))
#define UR_FLAG_CH1_CC1 BIT_MASK_MACRO(UR_BIT_CH1)
#define UR_FLAG_CH2_CC2 BIT_MASK_MACRO(UR_BIT_CH2)
//——————————位掩码 宏 结束————————————

//——————————队列区 开始——————————
//串口队列 发送led状态，oled状态，等等……
typedef struct{
    LED_State led_state;
    OLED_State oled_state;
}USARTMessage;

//按键队列，发送w25q64状态，等等……
typedef struct{
    W25Q64_State w25q64_state;
}KEYMessage;

//OLED队列 用于请求 OLED 显示某页面
typedef struct {
    OLED_State oled_state;   // 目标显示页面
    bool force_refresh;      // 是否强制刷新（即使页面没变）
    Sensor *Sensor;          // 指向要显示的传感器（电压页面 → &PS_Sensor，距离页面 → &UR_Sensor）
} OLEDDisplayRequest;

//UR PS消息通知队列
typedef struct{
    SensorEvent event;
}SensorNotifyMsg;
//——————————队列区 结束——————————


//——————————状态机区 开始——————————
//光敏传感器状态机  photosensitive sensor  PS
typedef enum{
    PS_Init,
    PS_Wait_ADC_JEOC,
    PS_Process,
    PS_Idle
}PS_IJ_State_Machine;

//超声波测距状态机  ultrasonic ranging  UR
typedef enum{
    UR_Send_Pulse_to_Trig,
    UR_Wait_Echo,
    UR_Process,
    UR_Idle
}UR_State_Machine;
//——————————状态机区 结束——————————

//APP状态 所有状态皆可放到此处
typedef struct{
    LED_State led;//led状态
    OLED_State oled;//oled状态
    W25Q64_State w25q64;//w25q64状态
    PS_IJ_State_Machine ps_state;//光敏传感器状态机状态
    UR_State_Machine ur_state;//超声波模块状态机

    float ps_voltage_v; //光敏电压 单位V 越大越暗
    float ur_distance_m; // 超声波距离 单位m

    float voltage_threshold;     // 电压阈值，单位 V
    float distance_threshold;    // 距离阈值，单位 m
}AppState;

extern KEYMessage keymessage;//按键队列
extern USARTMessage usartmessage;//串口队列
extern OLEDDisplayRequest req;//OLED队列
extern SensorNotifyMsg notify;//UR PS消息通知队列
extern volatile AppState g_state;//全局APP状态

#ifndef CPU_USAGE_ENABLED
#define My_TIM3_PeriodElapsedCallback(htim)
#endif

#endif //APP_COMMON_ENABLED

#endif

