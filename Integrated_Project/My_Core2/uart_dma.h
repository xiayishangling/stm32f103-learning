// uart_dma.h —— UART1 DMA 打印 + 接收
// UART_DMA_Printf: 带互斥锁的 DMA 发送（printf 替代方案）
// 注意：连续多次调用 UART_DMA_Printf 可能踩 tx_dma_buffer，建议拼成一条发
#ifndef INC_UART_DMA_H_
#define INC_UART_DMA_H_

#include "app_common.h"
#include "usart.h"

#define ADD_DMA_USART huart1            // 使用的串口
#define TX_DMA_BUFFER_SIZE 64           // 发送 DMA 缓冲区
#define RX_DMA_BUFFER_SIZE 64           // 接收 DMA 缓冲区

// DMA 打印宏：拿锁 → 等前次 DMA 发完 → sprintf → 启动 DMA → 放锁
#define UART_DMA_Printf(...) do{\
    osMutexAcquire(General_MutexHandle, osWaitForever);\
    while (__HAL_DMA_GET_COUNTER(ADD_DMA_USART.hdmatx) != 0);\
    int len = sprintf((char *)tx_dma_buffer,__VA_ARGS__);\
    HAL_UART_Transmit_DMA(&ADD_DMA_USART,tx_dma_buffer,len);\
    osMutexRelease(General_MutexHandle);\
}while(0)

extern uint8_t tx_dma_buffer[TX_DMA_BUFFER_SIZE];
extern uint8_t rx_dma_buffer[RX_DMA_BUFFER_SIZE];
extern uint8_t RX_RCV_Flish_Flag;

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size);

#endif
