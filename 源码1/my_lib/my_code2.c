#include "my_code2.h"
#include <stdio.h>
#include "stm32f10x.h"
#include "delay.h"



void my_one_code2(void)
{
    
}


//6.1 task
void key_process_led(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);

	GPIO_InitTypeDef GPIO_StructInit = {0};
	GPIO_StructInit.GPIO_Pin = GPIO_Pin_0;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_Out_PP;//led
	
	GPIO_Init(GPIOA,&GPIO_StructInit);//不能同时初始化！！！
	
	GPIO_StructInit.GPIO_Pin = GPIO_Pin_1;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_IPU;//button

	GPIO_Init(GPIOA,&GPIO_StructInit);

	while(1)
	{
		if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_1) == RESET)//按钮是上拉，读到低电平代表按下
		{
			Delay(20);
			GPIO_WriteBit(GPIOA,GPIO_Pin_0,Bit_SET);//推挽，高电平点亮
		}
		else if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_1) == SET)
		{
			Delay(20);
			GPIO_WriteBit(GPIOA,GPIO_Pin_0,Bit_RESET);
		}
	}
}

//6.2 task
/*
6.2 Task 问题汇总
一、你自己标注"???"发现的问题
第 80 行 — 忘记 RCC_APB2Periph_AFIO 的英文写法（已改正，写成 RCC_APB2Periph_AFIO 是对的）
第 87 行 — TX 引脚 PB6 的 Speed 原来写成了 2MHz，速度不够，应为 10MHz（已改正）
第 94 行 — RX 引脚 PB7 接收不需要 Speed，注释掉了（但未显式赋值，靠残留值碰巧可用）
第 112 行 — fputc 第二个参数原来写成了 FILE *p，应为 FILE *f（已改正）
第 136 行 — 超时计算原来用 expire_time 存了但没有判断（已改正为 Start_Time 方案）
第 149 行 — 超时判断原来写在 while 循环外面，搞错了，应该放到 while 里判断（已改正）
第 155 行 — 原来的 expire_time 是野参数（已放弃，改用 GetTick() - Start_Time > TimeOut）
第 167 行 — TC 标志等待那里忘记加 TimeOut != -1 的判断（已改正）
第 172 行 — 清除标志原来写死了 USART1，应该用 USARTx（已改正）
第 197 行 — USART_FLAG_RXNE 判断原来写成了 while，应为 if（已改正）
第 203 行 — GetTick 漏了 ()，写成了 GetTick（还未改）
二、我审查发现的新问题
第 81 行 GPIO_PinRemapConfig 参数错误 — 第一个参数写成了 GPIOA，应该是 GPIO_Remap_USART1。不改的话重映射根本不生效，USART1 还在 PA9/PA10 上
第 85 行和第 92 行 GPIO_StructInit 重复定义 — 同一个函数里三次声明 GPIO_InitTypeDef GPIO_StructInit = {0};（第 72、85、92 行），C89 标准下编译报错。你在 6.1 task 第 44 行自己注释了"不能同时初始化"，但这里又犯了同样的错
第 230 行 GPIOA 按钮引脚未初始化 — porcess_usart_sendandrecive() 只调用了 init_ledpc13_usartremap()，这个函数里没有初始化 GPIOA Pin 1。按钮初始化代码在 key_process_led() 里，但你没调用它。PA1 处于浮空输入，读到的值不确定
第 223 行和 228 行消息写反了 — 你的注释规格写的是发 1 灯亮收 LED ON，发 0 灯灭收 LED OFF。但代码里发的是 "LED NO" 和 "LED YES"，内容对不上规格
第 218 行阻塞等待无超时保护 — while(USART_GetFlagStatus(...) == RESET); 是死等，一旦没数据程序就卡死。你明明写了带超时的 receive_a_characterstring_usart()，却没用它
第 210 行函数名拼写错误 — porcess_usart_sendandrecive，porcess 应该是 process
第 84 行和第 91 行 GPIOB 时钟重复使能 — RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); 开了两次
三、剩下的隐患（你注释掉的残留问题）
第 137 行 — 注释里留下的 ret = process_usart_send_and_receive; 是函数地址赋值给 uint8_t，如果真的执行会截断指针，好在已注释掉
第 152 行 — 旧版 expire_time 方案的残留注释，逻辑已废弃，可以清理
*/
void init_ledpc13_usartremap(void)
{
	//pc13,板载led 初始化
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);
	GPIO_InitTypeDef GPIO_StructInit = {0};
	GPIO_StructInit.GPIO_Pin = GPIO_Pin_13;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_Init(GPIOC,&GPIO_StructInit);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	GPIO_StructInit.GPIO_Pin = GPIO_Pin_0;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_Out_PP;//led
	GPIO_Init(GPIOA,&GPIO_StructInit);//不能同时初始化！！！
	
	GPIO_StructInit.GPIO_Pin = GPIO_Pin_1;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_IPU;//button
	GPIO_Init(GPIOA,&GPIO_StructInit);

	//初始化重映射usart1的gpio pa9/10 ——> pb6/7
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);//???忘记rcc英文
	GPIO_PinRemapConfig(GPIO_Remap_USART1,ENABLE);//???重映射错误

	//TX PB6  //  RX PB7
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	GPIO_StructInit.GPIO_Pin = GPIO_Pin_6;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_10MHz;//？？？原来写成2了，速度不够
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_AF_PP;//推挽
	GPIO_Init(GPIOB,&GPIO_StructInit);

	//RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);//使能一次即可
	//GPIO_InitTypeDef GPIO_StructInit = {0};//不能重复定义
	GPIO_StructInit.GPIO_Pin = GPIO_Pin_7;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;//？？？接收不需要速度
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_IPU;//上拉
	GPIO_Init(GPIOB,&GPIO_StructInit);

	
	//初始化usart1
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);
	USART_InitTypeDef USART_StructInit = {0};

	USART_StructInit.USART_BaudRate = 115200;
	USART_StructInit.USART_WordLength = USART_WordLength_8b;
	USART_StructInit.USART_Parity = USART_Parity_No;
	USART_StructInit.USART_StopBits = USART_StopBits_1;
	USART_StructInit.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1,&USART_StructInit);
	USART_Cmd(USART1,ENABLE);
}

void init_USART1(void)
{
	//初始化usart1
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);
	USART_InitTypeDef USART_StructInit = {0};

	USART_StructInit.USART_BaudRate = 115200;
	USART_StructInit.USART_WordLength = USART_WordLength_8b;
	USART_StructInit.USART_Parity = USART_Parity_No;
	USART_StructInit.USART_StopBits = USART_StopBits_1;
	USART_StructInit.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1,&USART_StructInit);
	USART_Cmd(USART1,ENABLE);
}

//???函数名忘了
int fputc(int ch,FILE *f)//???写成p了
{
	while(USART_GetFlagStatus(USART1,USART_FLAG_TXE) == RESET);
	USART_SendData(USART1,(uint8_t)ch);
	return ch;
}

//带‘！！！’说明这个地方发现了错误，指的是没改正之前
//？？？函数头不记得要不要加上数组/忘记如何定义一个数组/忘记timeout 怎么来的
/**
 * @brief 带有超时保护的发送一串数据的函数
 * 
 * @param USARTx 串口x
 * @param pData 数组
 * @param Size 元素个数
 * @param TimeOut 跳出时间
 * @retval 返回1-超时  成功-0
 */
//TimeOut 的单位是毫秒，来自 GetTick() 的毫秒级计数。传 200 就是超时时间 200 毫秒。
//int32_t 不是用来存“负时间”，而是为了让参数多一种“无限等待”的指令。
//-1 就是这条指令的代号，在代码第一关就被识别并特殊处理，永远不参与时间计算。
int send_a_characterstring_usart(USART_TypeDef *USARTx,const uint8_t *pData,uint16_t Size,int32_t TimeOut)
{
	uint16_t i = 0;
	//？？？存档
	//uint32_t expire_time;//??? 野参数，500什么(原来写成timeout）
	//uint8_t ret = process_usart_send_and_receive;//???这样真的可以？

	if(pData == NULL || Size == 0 ) return USART_PARAM_ERR;//？？？有一个0就判断失败
	
	//expire_time = GetTick() + TimeOut;//???忘记判断了

	uint32_t Start_Time = GetTick();
	do
	{
		while(USART_GetFlagStatus(USARTx,USART_FLAG_TXE) == RESET)
		{
			//？？？上个版本存档
			// if(GetTick() > expire_time && TimeOut != -1)//？？？搞错了，应该是放到while里判断是否超时
			// {
			// 	return USART_TIMEOUT;
			// }

			if(TimeOut != -1 && (GetTick() - Start_Time) > TimeOut )//？？？搞错了，应该是放到while里判断是否超时
			{
				return USART_TIMEOUT;
			}
		}
		USART_SendData(USARTx,pData[i]);
		i++;
	}while(i < Size);


	while(USART_GetFlagStatus(USARTx,USART_FLAG_TC) == RESET)
	{
		if(TimeOut != -1 && GetTick() - Start_Time > TimeOut )//???忘记加TimeOut != -1了
		{
			return USART_TIMEOUT;
		}
	}
	USART_ClearFlag(USARTx,USART_FLAG_TC);//？？？都应该是USARTx

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

// 1. 发 `1` → 灯亮 + 收到 `LED ON`
// 2. 发 `0` → 灯灭 + 收到 `LED OFF`
// 3. 按按钮 → 收到 `Button Pressed!`（按一下只触发一次，不能刷屏）
void process_usart_send_and_recive(void)
{
	init_ledpc13_usartremap();
	// uint16_t ret = receive_a_characterstring_usart;
	// uint16_t Dada[100];
	GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
    while(1)
    {
        uint8_t ch;
        // 用带超时的接收函数收 1 个字节，超时 50ms 就跳过
        uint16_t ret = receive_a_characterstring_usart(USART1, &ch, 1, 50);
        if(ret == 0)  // 超时了，没收到数据，继续下一轮检查按钮
        {
			if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1) == RESET)
			{
				Delay(20);  // 消抖
				if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1) == RESET)  // 确认按下
				{
					send_a_characterstring_usart(USART1, "Button Pressed!", 15, 200);
					while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1) == RESET);  // 等待释放
					Delay(20);  // 释放消抖
				}
			}
        }
        else  // ret > 0, 收到了数据才处理字符//???之前是分开写的，没有else，导致一直发送
        {
        // 收到了数据
        if(ch == '1')
        {
            GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);
            send_a_characterstring_usart(USART1,"LED ON", 6, 200);
        }
        else if(ch == '0')
        {
            GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
            send_a_characterstring_usart(USART1, "LED OFF", 7, 200);
        }
        }
    }
}

