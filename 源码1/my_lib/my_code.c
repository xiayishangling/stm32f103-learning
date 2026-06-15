#include "vscode_defs.h" 
#include "my_code.h"
#include <stdio.h>
#include "stm32f10x.h"
#include "delay.h"

uint8_t a = 0;
uint32_t Flicker_time = 1000;



void my_one_code(void)
{
    
}

//6.3 task
void init_PC13_GPIO(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);
	GPIO_InitTypeDef GPIO_StructInit = {0};
	GPIO_StructInit.GPIO_Pin = GPIO_Pin_13;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_Init(GPIOC,&GPIO_StructInit);
}

void init_Button_GPIO(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	GPIO_InitTypeDef GPIO_StructInit = {0};
	GPIO_StructInit.GPIO_Pin = GPIO_Pin_1;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA,&GPIO_StructInit);
}

void init_I2C1_GPIO(void)
{
	//SCL-PB8 SCK-PB9
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_I2C1,ENABLE);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	
	GPIO_InitTypeDef GPIO_StructInit = {0};
	GPIO_StructInit.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_10MHz;//对应400k-留足余量/工业级  面包板限制-最好不填50MHz
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_Init(GPIOB,&GPIO_StructInit);
}

void init_I2C1(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1,ENABLE);//？？？ 要先开时钟
	RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1,ENABLE);
	RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1,DISABLE);
	
	I2C_InitTypeDef I2C_StructInit = {0};
	I2C_StructInit.I2C_DutyCycle = I2C_DutyCycle_2;// 2/1
	I2C_StructInit.I2C_Mode = I2C_Mode_I2C;//标准i2c模式
	I2C_StructInit.I2C_ClockSpeed =400000;//400k
	I2C_Init(I2C1,&I2C_StructInit);

	I2C_Cmd(I2C1,ENABLE);
}

/*
1.BUSY--总线忙  0--空闲  1--忙
2.START--发送起始位 1--开始  STOP--发送结束位  ENABLE(1)--结束
3.SB--起始位发送是否完成 0--未发送  1--发送完成
4.AF--应答失败 1--未收到ACK
5.ADDR--寻址成功  1--成功
6.TXE--发送数据寄存器是否为空   0--非空   1--空
7.RXNE--接收数据寄存器是否为空  0--空  1--非空
8.BTF--发送数据寄存器和移位寄存器是否为空  1--空
*/
/**
 * @brief I2C发送一串字节，带超时保护
 * 
 * @param I2Cx I2C1/2
 * @param ADDr 起始位地址
 * @param pData 要发送的数据 “xxx”
 * @param Size 数据个数
 * @param TimeOut 超时时间（ms）
 * @return AF_ERR -1//ADDr_ERR -2//Time_Out -3//I2C_Send_Sucess 1
 */
int I2C_Send_Bytes(I2C_TypeDef *I2Cx,uint8_t ADDr,const uint8_t *pData,uint32_t Size,int32_t TimeOut)
{
	if(pData == NULL || Size <= 0) return ERR;

	uint32_t Start_tick;
	//1.等待总线不忙
	Start_tick = GetTick();// ??? 获取时间不能放在循环内
	while(I2C_GetFlagStatus(I2Cx,I2C_FLAG_BUSY) == SET)//??? 1代表忙？
	{
		if(TimeOut != -1 && (GetTick() - Start_tick) > TimeOut)//防止计数器绕回
		{
			I2C_GenerateSTOP(I2Cx,ENABLE);
			return TIME_OUT;
		}
	}

	//2.发送开始位，SB检测
	I2C_GenerateSTART(I2Cx,ENABLE);
	Start_tick = GetTick();
	while(I2C_GetFlagStatus(I2Cx,I2C_FLAG_SB) == RESET)
	{
		if(TimeOut != -1 && (GetTick() - Start_tick) > TimeOut)
		{
			I2C_GenerateSTOP(I2Cx,ENABLE);
			return TIME_OUT;
		}
	}

	//等待 BUSY、等待 SB 的过程中，硬件绝对不会修改 AF 标志

	//3.发送数据前，清空AF
	I2C_ClearFlag(I2Cx,I2C_FLAG_AF);

	//4.发送从机地址，最后一位 0=writr 1=read
	//??? ADDr不是指针类型
	//如果调用方已经传入左移后的值（比如 0x60），那 & 0xFE 没问题，但容易混淆。
	//建议统一规定传入原始 7 位地址，内部自动移位，减少调用者出错概率。
	I2C_SendData(I2Cx,ADDr & 0xFE);
	//I2C_SendData(I2Cx, (ADDr << 1) & 0xFE);

	Start_tick = GetTick(); //??? 时间应该在循环外计算一次即可
	while(1)// ??? 之前括号里是：I2C_GetFlagStatus(I2Cx,I2C_FLAG_ADDR) == RESET
	{
		
		if(I2C_GetFlagStatus(I2Cx,I2C_FLAG_AF) == SET)
		{
			I2C_GenerateSTOP(I2Cx,ENABLE);
			I2C_ClearFlag(I2Cx,I2C_FLAG_AF);// ？？？ 之前未清除AF
			return AF_ERR;
		}

		if(I2C_GetFlagStatus(I2Cx,I2C_FLAG_ADDR) == SET)// ？？？之前写成reset ，以为是while
		{
			break;// ？？？ 要把这个if移到循环中，ADDR和AF双判
		}
		
		if(TimeOut != -1 && (GetTick() - Start_tick) > TimeOut)//？？？ 时间判断之前全写反了
		{
			I2C_GenerateSTOP(I2Cx,ENABLE);
			I2C_ClearFlag(I2Cx,I2C_FLAG_AF);
			return TIME_OUT;
		}
	}

	//5.清空ADDr，准备发送数据
	I2C_ReadRegister(I2Cx,I2C_Register_SR1);
	I2C_ReadRegister(I2Cx,I2C_Register_SR2);

	//6.发送数据
	
	for(uint16_t i = 0;i<Size;i++)
	{
		Start_tick = GetTick();
		while(1)
		{
			
			if(I2C_GetFlagStatus(I2Cx,I2C_FLAG_AF) == SET)
			{
				I2C_GenerateSTOP(I2Cx,ENABLE);
				I2C_ClearFlag(I2Cx,I2C_FLAG_AF);
				return AF_ERR;
			}

			if(I2C_GetFlagStatus(I2Cx,I2C_FLAG_TXE) == SET)//？？？ 以为是while
			{
				break;
			}
			
			if(TimeOut != -1 && (GetTick() - Start_tick) > TimeOut)
			{
				I2C_GenerateSTOP(I2Cx,ENABLE);
				I2C_ClearFlag(I2Cx,I2C_FLAG_AF);
				return TIME_OUT;
			}
		}

		I2C_SendData(I2Cx,pData[i]);

		//7.发送完成，检测AF和BTF
		/*
		意思是不用 【AF I2C_SendData(I2Cx,pData[i]); AF BTF - 循环】 这种结构，
		而是【AF I2C_SendData(I2Cx,pData[i]); last byte：AF BTF】
		理由：前 n-1 个字节：AF 检查被合并到下一轮等待 TXE 的循环里；
		最后一个字节：AF 检查被合并到等待 BTF 的循环里。
		*/
		if(i == Size - 1)//只检测最后一个数据的AF+BTF
		{
		Start_tick = GetTick();
		while(1)
		{
			
			if(I2C_GetFlagStatus(I2Cx,I2C_FLAG_AF) == SET)
			{
				I2C_GenerateSTOP(I2Cx,ENABLE);
				I2C_ClearFlag(I2Cx,I2C_FLAG_AF);
				return AF_ERR;
			}

			if(I2C_GetFlagStatus(I2Cx,I2C_FLAG_BTF) == SET)// ？？？
			{
				break;
			}

			if(TimeOut != -1 && (GetTick() - Start_tick) > TimeOut)
			{
				I2C_GenerateSTOP(I2Cx,ENABLE);
				I2C_ClearFlag(I2Cx,I2C_FLAG_AF);
				return TIME_OUT;
			}
		}
		}
		
	}
	
	//8.发送停止位
	I2C_GenerateSTOP(I2Cx,ENABLE);//释放总线，双线拉高

	return I2C_SEND_SUCCESS;

	//9.关闭总线 ？？？不需要 解释：为了下次可使用，再也不需要才能关掉
	//I2C_Cmd(I2Cx,DISABLE);
}

/**
 * @brief I2C 接收一串字节
 * 
 * @param I2Cx 
 * @param ADDr2 地址 最后一位写1
 * @param pBuffer 储存接收到的字符
 * @param Size2 接收几个字符
 * @param TimeOut2 超时时间 (ms)
 * @return TIME_OUT -3//ERR 0//I2C_RECIVE_SUCESS 2
 */
int I2C_Recive_Bytes(I2C_TypeDef *I2Cx,uint8_t ADDr2,uint8_t *pBuffer,uint32_t Size2,int32_t TimeOut2)
{
	if(pBuffer == NULL || Size2 <= 0) return ERR;

	//1.发送开始位，抢大概率空闲的总线
	I2C_GenerateSTART(I2Cx,ENABLE);
	uint32_t Start_tick2;

	Start_tick2 = GetTick();
	while(I2C_GetFlagStatus(I2Cx,I2C_FLAG_SB) == RESET)
	{
		if(TimeOut2 != -1 && GetTick() - Start_tick2 > TimeOut2)
		{
			I2C_GenerateSTOP(I2Cx,ENABLE);//？？？ 之前都忘了超时应该释放总线，只返回了值
			return TIME_OUT;
		}
	}

	//2.发送数据前，清空AF
	I2C_ClearFlag(I2Cx,I2C_FLAG_AF);
	
	//3.发送地址，并判断是否寻址成功,保证最后1位是1，与的结果是有1就1
	I2C_SendData(I2Cx,ADDr2 | 0x01);// ？？？ 写错了，写成||了
	//判断ADDR标志位
	Start_tick2 = GetTick();
	while(1)
	{
		if(I2C_GetFlagStatus(I2Cx,I2C_FLAG_AF) == SET)
		{
			I2C_ClearFlag(I2Cx,I2C_FLAG_AF);
			I2C_GenerateSTOP(I2Cx,ENABLE);
			return AF_ERR;
		}
		if(I2C_GetFlagStatus(I2Cx,I2C_FLAG_ADDR) == SET) break;

		if(TimeOut2 != -1 && GetTick() - Start_tick2 > TimeOut2)
		{
			I2C_ClearFlag(I2Cx,I2C_FLAG_AF);
			I2C_GenerateSTOP(I2Cx,ENABLE);
			return TIME_OUT;
		}
	}

	//1位
	if(Size2 == 1)// ??？ 之前忘记if/else if/else了 写成了三个if
	{
		I2C_ReadRegister(I2Cx,I2C_Register_SR1);
		I2C_ReadRegister(I2Cx,I2C_Register_SR2);
		I2C_AcknowledgeConfig(I2Cx,DISABLE);//send NCK
		I2C_GenerateSTOP(I2Cx,ENABLE);
		
		Start_tick2 = GetTick();
		while(I2C_GetFlagStatus(I2Cx,I2C_FLAG_RXNE) == RESET)//有数据才接收
		{
			if(TimeOut2 != -1 && GetTick() - Start_tick2 > TimeOut2)
			{
				I2C_GenerateSTOP(I2Cx,ENABLE);
				return TIME_OUT;
			}
		}
		pBuffer[0] = I2C_ReceiveData(I2Cx);
		return I2C_RECIVE_SUCCESS;//？？？ 之前忘记了
	}

	else if(Size2 == 2)
	{
		I2C_ReadRegister(I2Cx,I2C_Register_SR1);
		I2C_ReadRegister(I2Cx,I2C_Register_SR2);
		I2C_AcknowledgeConfig(I2Cx,ENABLE);//send ACK
		
		Start_tick2 = GetTick();
		while(I2C_GetFlagStatus(I2Cx,I2C_FLAG_RXNE) == RESET)
		{
			if(TimeOut2 != -1 && GetTick() - Start_tick2 > TimeOut2)
			{
				I2C_GenerateSTOP(I2Cx,ENABLE);
				return TIME_OUT;
			}
		}
		pBuffer[0] = I2C_ReceiveData(I2Cx);

		I2C_AcknowledgeConfig(I2Cx,DISABLE);//send NCK
		I2C_GenerateSTOP(I2Cx,ENABLE);
		
		Start_tick2 = GetTick();
		while(I2C_GetFlagStatus(I2Cx,I2C_FLAG_RXNE) == RESET)
		{
			if(TimeOut2 != -1 && GetTick() - Start_tick2 > TimeOut2)
			{
				I2C_GenerateSTOP(I2Cx,ENABLE);
				return TIME_OUT;
			}
		}
		pBuffer[1] = I2C_ReceiveData(I2Cx);
		return I2C_RECIVE_SUCCESS;
	}

	else
	{
		I2C_ReadRegister(I2Cx,I2C_Register_SR1);
		I2C_ReadRegister(I2Cx,I2C_Register_SR2);
		I2C_AcknowledgeConfig(I2Cx,ENABLE);//send ACK
		
		for(uint16_t i = 0;i<Size2-1;i++)
		{
			Start_tick2 = GetTick();//？？？ 之前这个放到了for外
			while(I2C_GetFlagStatus(I2Cx,I2C_FLAG_RXNE) == RESET)
			{
				if(TimeOut2 != -1 && GetTick() - Start_tick2 > TimeOut2)
				{
					I2C_GenerateSTOP(I2Cx,ENABLE);
					return TIME_OUT;
				}
			}
			pBuffer[i] = I2C_ReceiveData(I2Cx);
		}

		I2C_AcknowledgeConfig(I2Cx,DISABLE);//send NCK
		I2C_GenerateSTOP(I2Cx,ENABLE);
		
		Start_tick2 = GetTick();
		while(I2C_GetFlagStatus(I2Cx,I2C_FLAG_RXNE) == RESET)
		{
			if(TimeOut2 != -1 && GetTick() - Start_tick2 > TimeOut2)
			{
				I2C_GenerateSTOP(I2Cx,ENABLE);
				return TIME_OUT;
			}
		}
		pBuffer[Size2 - 1] = I2C_ReceiveData(I2Cx);
	}
	return I2C_RECIVE_SUCCESS;
}

void process_i2c_send_receive(void)
{
	init_I2C1_GPIO();
	init_PC13_GPIO();
	init_Button_GPIO();
	init_I2C1();

	uint8_t commands[]={0x00,0x8d,0x14,0xaf,0xa5};
	I2C_Send_Bytes(I2C1,0x78,commands,5,50);

	GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_SET);//off
	uint8_t rcvd;
	I2C_Recive_Bytes(I2C1,0x78,&rcvd,1,50);//从oled读一个字节放到rcvd之中
	if((rcvd & (0x01 << 6)) == 0)//D6 = 0时 屏幕点亮
	{
		while(1)
		{
			GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_RESET);
			if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_1) == RESET)
			{
				Delay(20);  // 消抖
				if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1) == RESET)  // 确认按下
				{
					while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1) == RESET);  // 等待释放
					Delay(20);  // 释放消抖
					break;
				}
			}
		}
		GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_SET);
	}
	else
	{
		GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_SET);
	}
}


//6.4 task
void init_SPI_GPIO(void)
{
	//PA4/5/6/7 pinremap PB3/4/5 PA15
	//释放PA15
	//串口1 || GPIO ABCD || SPI1在高速总线72MHz_APB2
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);

	//重映射SPI1_GPIO
	GPIO_PinRemapConfig(GPIO_Remap_SPI1,ENABLE);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB,ENABLE);
	GPIO_InitTypeDef GPIO_StructInit = {0};

	GPIO_StructInit.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_5;//SCK DO
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;//面包板，低速，保持边缘平缓，防止振铃和串扰
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB,&GPIO_StructInit);

	GPIO_StructInit.GPIO_Pin = GPIO_Pin_4;//DI
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;//防止未知变量；无效果
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB,&GPIO_StructInit);

	GPIO_StructInit.GPIO_Pin = GPIO_Pin_15;//CS
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_Out_PP;//普通io 负责片选从机
	GPIO_Init(GPIOA,&GPIO_StructInit);

	GPIO_WriteBit(GPIOA,GPIO_Pin_15,Bit_SET);//默认拉高，未选中从机
}

void init_SPI(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1,ENABLE);
	SPI_InitTypeDef SPI_StructInit = {0};

	SPI_StructInit.SPI_Direction = SPI_Direction_2Lines_FullDuplex;//双线全双工
	SPI_StructInit.SPI_Mode = SPI_Mode_Master;
	SPI_StructInit.SPI_CPHA = SPI_CPHA_1Edge;//00
	SPI_StructInit.SPI_CPOL = SPI_CPOL_Low;//00
	SPI_StructInit.SPI_DataSize = SPI_DataSize_8b;
	SPI_StructInit.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_StructInit.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
	SPI_StructInit.SPI_NSS = SPI_NSS_Soft;

	SPI_Init(SPI1,&SPI_StructInit);
	//SPI_Cmd(SPI1,ENABLE);  //??? spi无总开关 纠正：留到收发函数中开关
	SPI_NSSInternalSoftwareConfig(SPI1,SPI_NSSInternalSoft_Set);//要配置这个，使软件NSS生效
}

/**
 * @brief SPI收发函数
 * 
 * @param SPIx 
 * @param TXData 发送数组
 * @param RXData 接收数组
 * @param Size 元素个数
 * @param TimeOut 超时时间（ms）
 * @return Time_Out -3 // SPI_SUCCESS 3
 */
int SPI_Send_and_Receive(SPI_TypeDef *SPIx,const uint8_t *TXData,uint8_t *RXData,uint16_t Size,int32_t TimeOut)
{
	if(TXData == NULL || RXData == NULL || Size <= 0) return ERR;
	SPI_Cmd(SPIx,ENABLE);
	uint32_t Start_tick;

	
	for(uint16_t i = 0;i<Size;i++)
	{
		Start_tick = GetTick();
		while(SPI_I2S_GetFlagStatus(SPIx,SPI_I2S_FLAG_TXE) == RESET)//等到1，发送数据寄存器空，发送数据
		{	
			if(TimeOut != -1 && (GetTick() - Start_tick) > TimeOut)
			{
				SPI_Cmd(SPIx,DISABLE);
				return TIME_OUT;
			}
		}
		SPI_I2S_SendData(SPIx,TXData[i]);

		Start_tick = GetTick();
		while(SPI_I2S_GetFlagStatus(SPIx,SPI_I2S_FLAG_RXNE) == RESET)//等到1，接收数据寄存器非空，有数据，接收
		{	
			if(TimeOut != -1 && (GetTick() - Start_tick) > TimeOut)
			{
				SPI_Cmd(SPIx,DISABLE);
				return TIME_OUT;
			}
		}
		RXData[i] = SPI_I2S_ReceiveData(SPIx);
	}

	Start_tick = GetTick();
	while(SPI_I2S_GetFlagStatus(SPIx,SPI_I2S_FLAG_TXE) == RESET)//再次确认发送寄存器
	{	
		if(TimeOut != -1 && (GetTick() - Start_tick) > TimeOut)
		{
			SPI_Cmd(SPIx,DISABLE);
			return TIME_OUT;
		}
	}

	Start_tick = GetTick();
	while(SPI_I2S_GetFlagStatus(SPIx,SPI_I2S_FLAG_BSY) == SET)//检查总线
	{
		if(TimeOut != -1 && (GetTick() - Start_tick) > TimeOut)
		{
			SPI_Cmd(SPIx,DISABLE);
			return TIME_OUT;
		}
	}
		
		SPI_Cmd(SPIx,DISABLE);
		return SPI_SUCCESS;
}

/**
 * @brief 使用W25Q64保存一个字节
 * 
 * @param Byte 要保存的字节
 * @note 模仿老师教学版本，仅简单使用；万一SPI_Send_and_Receive（）出问题，未采用提示或者回滚
 */
void SPI_W25Q64_Save_Byte(uint8_t Byte)
{
	uint8_t Buffer[10];
	//1.写使能 0x06
	Buffer[0] = 0x06;//先写数据再片选，是工业化的习惯
	GPIO_WriteBit(GPIOA,GPIO_Pin_15,Bit_RESET);//选中
	SPI_Send_and_Receive(SPI1,Buffer,Buffer,1,50);
	GPIO_WriteBit(GPIOA,GPIO_Pin_15,Bit_SET);

	//2.扇擦除 0x20 起始位
	Buffer[0] = 0x20;
	Buffer[1] = 0x00;
	Buffer[2] = 0x00;
	Buffer[3] = 0x00;
	GPIO_WriteBit(GPIOA,GPIO_Pin_15,Bit_RESET);//??? 之前写到buffer0上
	SPI_Send_and_Receive(SPI1,Buffer,Buffer,4,50);
	GPIO_WriteBit(GPIOA,GPIO_Pin_15,Bit_SET);

	//3.等空闲 0x05
	while(1)
	{
		Buffer[0] = 0x05;
		GPIO_WriteBit(GPIOA,GPIO_Pin_15,Bit_RESET);//？？？ 之前开和关写到while外面了
		SPI_Send_and_Receive(SPI1,Buffer,Buffer,1,50);
		Buffer[0] = 0xff;//or 0x00
		SPI_Send_and_Receive(SPI1,Buffer,Buffer,1,50);
		GPIO_WriteBit(GPIOA,GPIO_Pin_15,Bit_SET);
		if((Buffer[0] & 0x01) == 0) break;//？？？ 之前写到关闭前面了 //当busy1->0时直接跳出while 之前写成1了
	}
	
	//4.写使能
	Buffer[0] = 0x06;
	GPIO_WriteBit(GPIOA,GPIO_Pin_15,Bit_RESET);//选中
	SPI_Send_and_Receive(SPI1,Buffer,Buffer,1,50);
	GPIO_WriteBit(GPIOA,GPIO_Pin_15,Bit_SET);

	//5.页编程 0x02 起始位 数据位
	
	Buffer[0] = 0x02;
	Buffer[1] = 0x00;
	Buffer[2] = 0x00;
	Buffer[3] = 0x00;
	Buffer[4] = Byte;
	GPIO_WriteBit(GPIOA,GPIO_Pin_15,Bit_RESET);
	SPI_Send_and_Receive(SPI1,Buffer,Buffer,5,50);
	GPIO_WriteBit(GPIOA,GPIO_Pin_15,Bit_SET);

	//6.等空闲
	while(1)
	{
		Buffer[0] = 0x05;
		GPIO_WriteBit(GPIOA,GPIO_Pin_15,Bit_RESET);//？？？ 之前开和关写到while外面了
		SPI_Send_and_Receive(SPI1,Buffer,Buffer,1,50);
		Buffer[0] = 0xff;//or 0x00
		SPI_Send_and_Receive(SPI1,Buffer,Buffer,1,50);
		GPIO_WriteBit(GPIOA,GPIO_Pin_15,Bit_SET);
		if((Buffer[0] & 0x01) == 0) break;//？？？ 之前写到关闭前面了
	}
}

/**
 * @brief W25Q64读取一个字节
 * 
 * @return Buffer[0]
 * @note 仅简单使用
 */
uint8_t SPI_W25Q64_Read_Byte(void)
{
	uint8_t Buffer[10];
	//0x03 起始位
	GPIO_WriteBit(GPIOA,GPIO_Pin_15,Bit_RESET);
	Buffer[0] = 0x03;
	Buffer[1] = 0x00;
	Buffer[2] = 0x00;
	Buffer[3] = 0x00;
	SPI_Send_and_Receive(SPI1,Buffer,Buffer,4,50);//??? 接收同理，发送第一个是无意义的
	Buffer[0] = 0xff;
	SPI_Send_and_Receive(SPI1,Buffer,Buffer,1,50);
	GPIO_WriteBit(GPIOA,GPIO_Pin_15,Bit_SET);
	return Buffer[0];
}

void Process_SPI(void)
{
	init_SPI_GPIO();
	init_SPI();
	init_PC13_GPIO();
	init_Button_GPIO();

	GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_SET);

	SPI_W25Q64_Save_Byte(0x12);
	a = SPI_W25Q64_Read_Byte();
	if(a == 0x12)
	{
		while(1)
		{
			GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_RESET);
			if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_1) == RESET)
			{
				Delay(20);  // 消抖
				if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1) == RESET)  // 确认按下
				{
					while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1) == RESET);  // 等待释放
					Delay(20);  // 释放消抖
					break;
				}
			}
		}
		GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_SET);
	}
	else
	{
		GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_SET);
	}
}


























void init_NVIC_PC13_Gpio(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);
	GPIO_InitTypeDef GPIO_StructInit = {0};
	GPIO_StructInit.GPIO_Pin = GPIO_Pin_13;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_Init(GPIOC,&GPIO_StructInit);
}

void init_NVIC_Button_1_2_GPIO(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	GPIO_InitTypeDef GPIO_StructInit = {0};
	GPIO_StructInit.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_IPU;//button 1 2
	GPIO_Init(GPIOA,&GPIO_StructInit);
}

void init_NVIC_Usart_Gpio(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_USART1,ENABLE);

	//PB6 PB7
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);

	GPIO_InitTypeDef GPIO_StructInit = {0};

	GPIO_StructInit.GPIO_Pin = GPIO_Pin_6;//TX
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB,&GPIO_StructInit);

	GPIO_StructInit.GPIO_Pin = GPIO_Pin_7;//RX
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB,&GPIO_StructInit);
}

void init_NVIC_Usart(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);

	USART_InitTypeDef USART_StructInit = {0};

	USART_StructInit.USART_BaudRate = 115200;
	USART_StructInit.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_StructInit.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_StructInit.USART_Parity = USART_Parity_No;
	USART_StructInit.USART_StopBits = USART_StopBits_1;
	USART_StructInit.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART1,&USART_StructInit);

	USART_Cmd(USART1,ENABLE);

	USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);//检测标志位触发中断
}

void init_NVIC(void)
{
	// Time clock have open
	NVIC_InitTypeDef NVIC_StructInit = {0};
	NVIC_StructInit.NVIC_IRQChannel = USART1_IRQn;
	NVIC_StructInit.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_StructInit.NVIC_IRQChannelSubPriority = 0;
	NVIC_StructInit.NVIC_IRQChannelCmd = ENABLE;// ???之前写enable，不确定

	NVIC_Init(&NVIC_StructInit);
}

void LED_flicker(void)
{
	GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_RESET);
	Delay(Flicker_time);
	GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_SET);
	Delay(Flicker_time);
}

// void USART1_IRQHandler(void)
// {
// 	if(USART_GetFlagStatus(USART1,USART_FLAG_RXNE) == SET)
// 	{
// 		uint16_t i = USART_ReceiveData(USART1);
// 		if(i == '1')
// 		{
// 			Flicker_time = 500;
// 		}
// 		else if(i == '2')
// 		{
// 			Flicker_time = 50;
// 		}
// 		else if(i == '0')
// 		{
// 			while(1)
// 			{
// 			GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_RESET);
// 			if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1) == RESET)
// 				{
// 					Delay(20);  // 消抖
// 					if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1) == RESET)  // 确认按下
// 					{
// 						while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1) == RESET);  // 等待释放
// 						Delay(20);  // 释放消抖
// 						break;
// 					}
// 				}
// 			}
// 			GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_SET);
// 		}
// 		else {}
// 	}
// }

void Process_NVIC_Control_LedFlicker(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//函数头确定分组
	init_NVIC_PC13_Gpio();
	init_NVIC_Button_1_2_GPIO();
	init_NVIC_Usart_Gpio();
	init_NVIC_Usart();
	init_NVIC();
	while (1)
	{
		LED_flicker();
	}
	
}

void init_EXTI(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA,GPIO_PinSource1); //？？？之前忘记函数名了 且不熟悉用法
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA,GPIO_PinSource2);

	//配置NVIC
	NVIC_InitTypeDef NVIC_StructInit = {0};

	NVIC_StructInit.NVIC_IRQChannel = EXTI1_IRQn;
	NVIC_StructInit.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_StructInit.NVIC_IRQChannelSubPriority = 0;
	NVIC_StructInit.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_StructInit);

	NVIC_StructInit.NVIC_IRQChannel = EXTI2_IRQn;
	NVIC_StructInit.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_StructInit.NVIC_IRQChannelSubPriority = 0;
	NVIC_StructInit.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_StructInit);

	//配置EXTI
	EXTI_InitTypeDef EXTI_StructInit = {0};
	EXTI_StructInit.EXTI_Line = EXTI_Line1 | EXTI_Line2;
	EXTI_StructInit.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_StructInit.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_StructInit.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_StructInit);

}

void EXTI_Button_Control_PC13(void)
{
	if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_1) == RESET)//左按钮点亮
	{
		GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_RESET);//light
		Delay(20);
	}
	else if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_2) == RESET)//右按钮熄灭
	{
		GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_SET);
		Delay(20);
	}
}

// void EXTI1_IRQHandler(void)
// {
// 	if(EXTI_GetFlagStatus(EXTI_Line1) == SET)
// 	{
// 		EXTI_ClearFlag(EXTI_Line1);
// 		GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_RESET);//保持前一个动作，直到上升沿触发,即松开按钮，才点亮
// 	}
// }

// void EXTI2_IRQHandler(void)
// {
// 	if(EXTI_GetFlagStatus(EXTI_Line2) == SET)
// 	{
// 		EXTI_ClearFlag(EXTI_Line2);
// 		GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_SET);
// 	}
// }

void Process_EXTI(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	init_EXTI();
	init_NVIC_PC13_Gpio();
	init_NVIC_Button_1_2_GPIO();
	GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_SET);//默认熄灭
	while(1)
	{
		//EXTI_Button_Control_PC13();  //？？？ 轮询会直接执行，中断无法生效
	}
}
