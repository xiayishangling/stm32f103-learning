#ifndef _MY_CODE2_H_
#define _MY_CODE2_H_

#include "vscode_defs.h"
#include "stm32f10x.h"
#include "my_code.h"
#include "delay.h"
#include "oled.h"
#include <stdio.h>
#include "math.h"
#include "string.h"
#include "usart.h"


#define INIT_SYSTEM_ERR -1
#define INIT_SYSTEM_SUCCESS 0

#define SYSTEM_HSE_TIMEOUT 1
#define SYSTEM_PLL_TIMEOUT 2
#define SYSTEM_SELECT_ERR 3

#define LED_ON 4
#define LED_OFF 5

#define RANG_OK 6
#define RANG_NO 7
#define RANG_ERR 8

#define CC1_OK 9
#define CC2_OK 10

/* --- 全局变量 extern 声明（原定义移到了 mycode2.c） --- */
extern volatile uint8_t  System_State;
extern volatile uint8_t  system_err_flag;
extern volatile uint32_t currentTick;
extern volatile uint32_t timer3_overflow_count;
extern volatile uint8_t  LED_MODE;
extern volatile uint8_t  Rang_State;
extern volatile uint8_t  capture_state;

typedef enum {trig_send_begin, wait_echo, process, interval} state_t;
extern state_t state;

extern volatile uint16_t ccr1, ccr2;
extern volatile uint32_t start_time;
extern volatile uint32_t time_led;

typedef enum {ms_init, wait_trigger, ms_process, inter_val} measure_t;
extern measure_t         measure_state;
extern volatile uint32_t ms_time;
extern volatile uint16_t ms_cc1, ms_cc2;
extern volatile uint8_t  ms_cc_flag;

/* --- 函数声明 --- */
void my_one_code2(void);
void Init_USART1(void);

//6.7时钟部分
int  My_System_Init(void);
void Process_systeminit(void);

//6.7定时器部分
void init_PC13_GPIO(void);//纯初始化板载LED PC13
void My_Delay_1(uint32_t ms);
void Init_TIM2(void);          //启动ms级定时器
void Process_mydelay(void);
void TIM2_IRQHandler(void);

//6.7呼吸灯部分
void init_CCR1_Compare_GPIO(void); //捕获 比较 寄存器
void init_CCR1_Compare(void);
void Process_Breathing_lamp(void);

//6.9 超声波测距部分
void ultrasonic_ranging_state_machine(void);
void LED_state_machine(void);
void Process_ultrasonic_ranging(void);
void Init_Timebase_InputCapture(void);
void us_TIM(void);              //启动us级定时器
void TIM3_IRQHandler(void);     //us
void TIM1_CC_IRQHandler(void);

//6.10 测量一个PWM信号 Measure_PWM_signal
void Init_USART1_Measure_PWM_signal(void);
void Init_OC_generatePWM_Measure_PWM_signal(void);
void Init_IC_measurePWM_Measure_PWM_signal(void);
void Process_Measure_PWM_signal(void);
void sin_flicker(void);
void Measure_PWM_signal_state_machine(void);

void TIM1_TRG_COM_IRQHandler(void);
void TIM1_CC_IRQHandler(void);
void TIM1_NVIC_Measure_PWM_signal(void);

#endif
