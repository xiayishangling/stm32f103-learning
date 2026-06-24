// usart_task.h —— 串口接收 + CLI 命令入口
#ifndef INC_USART_TASK_H_
#define INC_USART_TASK_H_

#include "app_common.h"

#ifdef USART_MODULE_ENABLED
void vIP_USARTTask(void *argument);                       // 串口接收 → CLI_Parse
void UART1_Receive_Start(void);                            // 开启中断接收
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);  // 接收完成回调
extern uint8_t rxData;                                     // 接收字节缓冲区
#endif

#endif
