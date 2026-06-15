#ifndef _MY_CODE2_H_
#define _MY_CODE2_H_
#include "stm32f10x.h"
#include <stdio.h>

#define USART_OK        0
#define USART_TIMEOUT   1
#define USART_PARAM_ERR 2 //parameter 参数

void my_one_code2(void);

//6.1 task
void key_process_led(void);

//6.2 task usart
void init_ledpc13_usartremap(void);
void init_USART1(void);
int fputc(int ch,FILE *f);//???函数名忘了
int send_a_characterstring_usart(USART_TypeDef *USARTx,const uint8_t *pData,uint16_t Size,int32_t TimeOut);
uint16_t receive_a_characterstring_usart(USART_TypeDef *USARTx,uint8_t *outpData,uint16_t Size2,int32_t TimeOut2);
void process_usart_send_and_recive(void);





#endif

