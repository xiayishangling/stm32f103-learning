// uart_dma.c —— UART1 缓冲区 + fputc/fgetc + DMA 接收回调
//   tx_dma_buffer: UART_DMA_Printf 使用的发送缓冲区
//   rx_dma_buffer: DMA 空闲中断接收缓冲区
//   fputc/fgetc  : printf/scanf 重定向到 UART1
#include "uart_dma.h"

uint8_t tx_dma_buffer[TX_DMA_BUFFER_SIZE] = {0};
uint8_t rx_dma_buffer[RX_DMA_BUFFER_SIZE] = {0};
uint8_t RX_RCV_Flish_Flag = 0;

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if(huart == &ADD_DMA_USART)
    {
        if(RX_RCV_Flish_Flag == 0)
        {
            RX_RCV_Flish_Flag = 1;
            if(Size < RX_DMA_BUFFER_SIZE)
            {
                rx_dma_buffer[Size] = '\0';
            }
            HAL_UARTEx_ReceiveToIdle_DMA(&ADD_DMA_USART,rx_dma_buffer,RX_DMA_BUFFER_SIZE);
        }
    }
}

int fputc(int ch,FILE *f)
{
    HAL_UART_Transmit(&ADD_DMA_USART,(uint8_t *)&ch,1,100);
    return ch;
}

int fgetc(FILE *f)
{
    uint8_t ch;
    HAL_UART_Receive(&ADD_DMA_USART,&ch,1,100);
    return ch;
}

