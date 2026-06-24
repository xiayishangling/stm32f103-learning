// usart_task.c —— 串口1 接收任务：
//   中断收到一个字节 → 释放信号量 → 此任务拼成字符串 → 遇 \\r\\n 交给 CLI_Parse 解析

#include "usart_task.h"
#include "cli.h"

#ifdef USART_MODULE_ENABLED

uint8_t rxData;

// 每收到一个字节，拼入 cmdbuff[32]，遇回车换行则调用 CLI_Parse
// CLI_Parse 查命令表分发到对应 cmd_xxx() 处理函数
void vIP_USARTTask(void *argument)
{
    UART1_Receive_Start();
    char cmdbuff[32];
    uint16_t index = 0;
    uint8_t byte;
    for(;;)
    {
        if(osSemaphoreAcquire(USART_CountingSemHandle,osWaitForever) == osOK)
        {

            byte = rxData;
            if(byte == '\r' || byte == '\n')
            {
                if(index > 0)
                {
                    cmdbuff[index] = '\0';
                    //CLI表出问题，也能使用1/2/3简单判定串口
                    // if(strcmp(cmdbuff,"1") == 0)
                    // {
                    //     usartmessage.led_state = LED_State_ON;
                    //     usartmessage.oled_state = OLED_State_Display1;
                    // }
                    // else if(strcmp(cmdbuff,"2") == 0)
                    // {
                    //     usartmessage.led_state = LED_State_OFF;
                    //     usartmessage.oled_state = OLED_State_Display2;                        
                    // }
                    // else if((strcmp(cmdbuff,"3") == 0))
                    // {
                    //     usartmessage.led_state = LED_State_Flicker;
                    //     usartmessage.oled_state = OLED_State_Display3;//恢复开始界面                       
                    // }
                    // else
                    // {
                    //     UART_DMA_Printf("Invalid_input\r\n");
                    // }
                    // if(osMessageQueuePut(USART_QueueHandle,&usartmessage,0,pdMS_TO_TICKS(100)) != osOK)
                    // {
                    //     UART_DMA_Printf("usartQueue_err\r\n");
                    // }
                    CLI_Parse(cmdbuff);
                    index = 0;
                    memset(cmdbuff,0,sizeof(cmdbuff));
                }
            }
            else
            {
                if(index < (sizeof(cmdbuff) - 1))
                {
                    cmdbuff[index++] = byte;
                }
                else
                {
                    index = 0;
                    memset(cmdbuff,0,sizeof(cmdbuff));
                }
            }
            UART1_Receive_Start();//接收完成再次开启中断，准备接收
        }
    }
}

void UART1_Receive_Start(void)
{
    HAL_UART_Receive_IT(&huart1,&rxData,1);
}

// 串口1 接收完成中断回调 —— 只做一件事：释放计数信号量通知 USART 任务
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart -> Instance == USART1)
    {
        osSemaphoreRelease(USART_CountingSemHandle);
    }
}

#endif //USART_MODULE_ENABLED

