#ifndef _MY_CODE_H_
#define _MY_CODE_H_
#include "stm32f10x.h"
#include <stdio.h>

#define ERR 0

#define AF_ERR -1
#define ADDR_ERR -2
#define TIME_OUT -3
#define I2C_SEND_SUCCESS 1 // ??? 知道了标志位统一大写这个规范
#define I2C_RECIVE_SUCCESS 2

#define SPI_SUCCESS 3

void my_one_code(void);//类型名要写清楚，不写是未指定，不符合代码编写规范

//带‘！！！’说明这个地方发现了错误
//6.3 task
void init_PC13_GPIO(void);
void init_I2C1_GPIO(void);
void init_I2C1(void);
int I2C_Send_Bytes(I2C_TypeDef *I2Cx,uint8_t ADDr,const uint8_t *pData,uint32_t Size,int32_t TimeOut);
int I2C_Recive_Bytes(I2C_TypeDef *I2Cx,uint8_t ADDr2,uint8_t *pBuffer,uint32_t Size2,int32_t TimeOut2);
void process_i2c_send_receive(void);


//6.4 task
void init_SPI_GPIO(void);
void init_SPI(void);
int SPI_Send_and_Receive(SPI_TypeDef *SPIx,const uint8_t *TXData,uint8_t *RXData,uint16_t Size,int32_t TimeOut);
void SPI_W25Q64_Save_Byte(uint8_t Byte);
uint8_t SPI_W25Q64_Read_Byte(void);
void Process_SPI(void);


//task 6.5
void init_NVIC_PC13_Gpio(void);
void init_NVIC_Button_1_2_GPIO(void);
void init_NVIC_Usart_Gpio(void);
void init_NVIC_Usart(void);
void init_NVIC(void);
void LED_flicker(void);
//void USART1_IRQHandler(void);
void Process_NVIC_Control_LedFlicker(void);

void init_EXTI(void);
void EXTI_Button_Control_PC13(void);
//void EXTI1_IRQHandler(void);
//void EXTI2_IRQHandler(void);
void Process_EXTI(void);


#endif

