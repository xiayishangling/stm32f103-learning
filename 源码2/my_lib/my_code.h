#ifndef _MY_CODE_H_
#define _MY_CODE_H_
#include "stm32f10x.h"
#include "delay.h"
#include "oled.h"
#include <stdio.h>
#include "math.h"
#include "my_code2.h"

#define USART_OK        0
#define USART_TIMEOUT   1
#define USART_PARAM_ERR 2 //parameter 参数

void my_one_code(void);//类型名要写清楚，不写是未指定，不符合代码编写规范

void pratice_init_PWM(void);

void Init_GPIO_USART_For_ultrasonic_ranging(void);
int fputc(int ch,FILE *f);
int send_a_characterstring_usart(USART_TypeDef *USARTx,const uint8_t *pData,uint16_t Size,int32_t TimeOut);
uint16_t receive_a_characterstring_usart(USART_TypeDef *USARTx,uint8_t *outpData,uint16_t Size2,int32_t TimeOut2);



#endif

