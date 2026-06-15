#include "my_code.h"



void my_one_code(void)
{
    
}

void pratice_init_PWM(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1,ENABLE);
    
    TIM_CtrlPWMOutputs(TIM1,ENABLE);//??? 之前忘记打开MOE,主要 输出 使能 在CCR的(捕获/比较 寄存器中的)
    
    TIM_OCInitTypeDef TIM_OCStructInit = {0};
    TIM_OCStructInit.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCStructInit.TIM_OCNPolarity = TIM_OCNPolarity_High;
    TIM_OCStructInit.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCStructInit.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
    TIM_OCStructInit.TIM_OCIdleState =TIM_OCIdleState_Reset;
    TIM_OCStructInit.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCStructInit.TIM_OutputNState = TIM_OutputNState_Enable;
    TIM_OCStructInit.TIM_Pulse = 0;
    TIM_OC1Init(TIM1,&TIM_OCStructInit);

    TIM_TimeBaseInitTypeDef TIM_timebase_StructInit = {0};
    TIM_timebase_StructInit.TIM_Prescaler = 71;
    TIM_timebase_StructInit.TIM_Period = 999;
    TIM_timebase_StructInit.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_timebase_StructInit.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1,&TIM_timebase_StructInit);


    TIM_Cmd(TIM1,ENABLE);//??? 之前忘记总开关
}


void Init_GPIO_USART_For_ultrasonic_ranging(void)
{
	//pc13,板载led 初始化
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC| RCC_APB2Periph_AFIO |RCC_APB2Periph_USART1,ENABLE);
	
    GPIO_InitTypeDef GPIO_StructInit = {0};
	
    GPIO_StructInit.GPIO_Pin = GPIO_Pin_13;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_Init(GPIOC,&GPIO_StructInit);
	
	GPIO_StructInit.GPIO_Pin = GPIO_Pin_1;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_Out_PP;//trig 输出一个大于10us的脉冲，开启工作
	GPIO_Init(GPIOA,&GPIO_StructInit);

    GPIO_StructInit.GPIO_Pin = GPIO_Pin_8;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_IPD;//echo 输入捕获 采用输入浮空 or 输入下拉-对应空闲状态低电压
	GPIO_Init(GPIOA,&GPIO_StructInit);

	//初始化重映射usart1的gpio pa9/10 ——> pb6 TX/7 RX
	GPIO_PinRemapConfig(GPIO_Remap_USART1,ENABLE);
	
    GPIO_StructInit.GPIO_Pin = GPIO_Pin_6;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_AF_PP;//推挽
	GPIO_Init(GPIOB,&GPIO_StructInit);

	GPIO_StructInit.GPIO_Pin = GPIO_Pin_7;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_IPU;//上拉
	GPIO_Init(GPIOB,&GPIO_StructInit);

	
	//初始化usart1
	USART_InitTypeDef USART_StructInit = {0};
	USART_StructInit.USART_BaudRate = 115200;
	USART_StructInit.USART_WordLength = USART_WordLength_8b;
	USART_StructInit.USART_Parity = USART_Parity_No;
	USART_StructInit.USART_StopBits = USART_StopBits_1;
	USART_StructInit.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1,&USART_StructInit);
	USART_Cmd(USART1,ENABLE);
}

int fputc(int ch,FILE *f)
{
	while(USART_GetFlagStatus(USART1,USART_FLAG_TXE) == RESET);
	USART_SendData(USART1,(uint8_t)ch);
	return ch;
}

/**
 * @brief 带有超时保护的发送一串数据的函数
 * 
 * @param USARTx 串口x
 * @param pData 数组
 * @param Size 元素个数
 * @param TimeOut 跳出时间
 * @retval 返回1-超时  成功-0
 */
int send_a_characterstring_usart(USART_TypeDef *USARTx,const uint8_t *pData,uint16_t Size,int32_t TimeOut)
{
	uint16_t i = 0;

	if(pData == NULL || Size == 0 ) return USART_PARAM_ERR;//？？？有一个0就判断失败

	uint32_t Start_Time = GetTick();
	do
	{
		while(USART_GetFlagStatus(USARTx,USART_FLAG_TXE) == RESET)
		{
			if(TimeOut != -1 && (GetTick() - Start_Time) > TimeOut )
			{
				return USART_TIMEOUT;
			}
		}
		USART_SendData(USARTx,pData[i]);
		i++;
	}while(i < Size);


	while(USART_GetFlagStatus(USARTx,USART_FLAG_TC) == RESET)
	{
		if(TimeOut != -1 && GetTick() - Start_Time > TimeOut )
		{
			return USART_TIMEOUT;
		}
	}
	USART_ClearFlag(USARTx,USART_FLAG_TC);

	return USART_OK;//成功

}

/**
 * @brief 串口接收字符串，带超时保护
 * 
 * @param USARTx 
 * @param outpData 
 * @param Size2 
 * @param TimeOut2 
 * @return 返回接收到几个字符
 */
uint16_t receive_a_characterstring_usart(USART_TypeDef *USARTx,uint8_t *outpData,uint16_t Size2,int32_t TimeOut2)
{
    uint16_t i = 0;
    uint32_t Start_Time = GetTick();
    do {
        if(USART_GetFlagStatus(USARTx, USART_FLAG_RXNE) == SET) {
            outpData[i++] = USART_ReceiveData(USARTx);
            if(i == Size2) break;
        }
    } while(TimeOut2 < 0 || (GetTick() - Start_Time) <= TimeOut2);
    return i;//此函数返回值是数据个数，即size
}



