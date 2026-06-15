#include "vscode_defs.h"  // 先加宏定义，再包含其他头文件
#include "stm32f10x.h"
#include <stdio.h>//重写fputc要用
#include "my_code.h"
#include "my_code2.h"
#include "delay.h"
#include "oled.h"

#define INIT_SYSTEM_ERR -1
#define INIT_SYSTEM_SUCCESS 0

#define LED_MODE_ON 1
#define LED_MODE_OFF 2
#define LED_MODE_BLINK 3 //闪烁

volatile uint8_t led_mode = LED_MODE_BLINK;//默认状态是闪烁

volatile uint8_t flag_save = 0;//FLASH W25Q64 1 保存
volatile uint8_t flag_load = 0;//FLASH W25Q64 1 装载

volatile uint8_t oled_update = 0;//OLED 是否更新oled状态 1更新
volatile uint8_t oled_display_state = 0;//OLED 1-发送ON 0-发送OFF

volatile uint8_t LED_State = 0; //LED 闪烁标志位 1-亮 0灭
volatile uint32_t last_toggle_time = 0; //LED 上次 切换 时间

OLED_TypeDef oled;
I2C_TypeDef *g_current_i2c = I2C1;//I2C接口 global（全局）

uint32_t Flicker_Time = 1000;//1s

//task 6.6 comprehensive exercises
int I2C_OLED_Display(uint8_t addr, const uint8_t *pdata, uint16_t size);
void init_OLDE(void);
void Process_I2C_OLED_Display(void);
void OLED_Show_LED_Status(uint8_t is_on);
void PC13_LED_Flicker(void);
void CE_Init_NVIC_For_USART(void);// CE == comprehensive exercises
void CE_Init_EXTI_For_Button(void);
void USART1_IRQHandler(void);
void EXTI1_IRQHandler(void);
void EXTI2_IRQHandler(void);
void PROCESS(void);
int My_System_Init(void);


int main(void)
{
	//key_process_led();
	//process_usart_send_and_recive();
	//process_i2c_send_receive();
	//Process_SPI();
	//Process_NVIC_Control_LedFlicker();
	//Process_EXTI();
	PROCESS();
	while(1)
	{
	}
}

/*
//已经注释掉了starup中的系统自带System Init函数
; Reset handler
Reset_Handler    PROC
                EXPORT  Reset_Handler             [WEAK]
    IMPORT  __main
    IMPORT  S ystemInit
;                 LDR     R0, =S ystemInit
;                 BLX     R0
                LDR     R0, =__main
                BX      R0
                ENDP*/
int My_System_Init(void)
{
	volatile uint32_t TimeOut;
	// 【核心：当前SYSCLK为默认8MHz（未执行System_Init），需先配置Flash适配后续72MHz高频】
	// Flash读写速度慢，直接超频到72MHz会导致CPU读指令出错，需提前做2个适配：
	// 1. 开启Flash指令预取缓冲区（提前缓存指令，提升读取效率）
	FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);//要用专用函数名
	// 2. 设置Flash访问延迟：72MHz对应延迟2个时钟周期（0-24MHz→0；24-48MHz→1；48-72MHz→2）
	FLASH_SetLatency(FLASH_Latency_2);

	RCC_HSEConfig(RCC_HSE_ON);//？？？ 之前写成enable了
	TimeOut = 0x0500;//1-1.6us 晶振起振要1-2us 此时间判断超时可等待稳定+不会等太久
	while((RCC_GetFlagStatus(RCC_FLAG_HSERDY) == RESET) && (--TimeOut));//括号要注意，优先级可能会出错
	if(TimeOut == 0)
	{
		RCC_HSEConfig(RCC_HSE_OFF);//关闭HES
		RCC_PLLCmd(DISABLE);
		//警示灯亮;暂不打算实现
		return INIT_SYSTEM_ERR;
	}

	/*  @arg RCC_PLLSource_HSI_Div2: 选择 HSI 振荡器时钟 2 分频作为 PLL 时钟输入
        @arg RCC_PLLSource_HSE_Div1: 选择 HSE 振荡器时钟（不分频）作为 PLL 时钟输入
        @arg RCC_PLLSource_HSE_Div2: 选择 HSE 振荡器时钟 2 分频作为 PLL 时钟输入   */
	RCC_PLLConfig(RCC_PLLSource_HSE_Div1,RCC_PLLMul_9);
	RCC_PLLCmd(ENABLE);
	TimeOut = 0x0500;
	while((RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET) && (--TimeOut));//== 的优先级高于 &&
	if(TimeOut == 0)
	{
		RCC_HSEConfig(RCC_HSE_OFF);//关闭HES
		RCC_PLLCmd(DISABLE);
		//警示灯亮
		return INIT_SYSTEM_ERR;
	}

	RCC_HCLKConfig(RCC_SYSCLK_Div1);//配置AHB分频器的分频系数 Advanced High-Speed Bus Prescaler
	RCC_PCLK1Config(RCC_HCLK_Div2);//配置APB1分频器的分频系数 Advanced Peripheral Bus 1 Prescaler
	RCC_PCLK2Config(RCC_HCLK_Div1);//配置APB2分配器的分频系数

	/*  - 0x00: HSI used as system clock
        - 0x04: HSE used as system clock
        - 0x08: PLL used as system clock  */
	RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
	TimeOut = 0x0500;
	while((RCC_GetSYSCLKSource() != 0x08) && (--TimeOut));//？？？ 之前没理解
	if(TimeOut == 0)
	{
		RCC_HSEConfig(RCC_HSE_OFF);//关闭HES
		RCC_PLLCmd(DISABLE);
		RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
		while(1);//死锁，放弃治疗，等看门狗重置整个系统
	}
	return INIT_SYSTEM_SUCCESS;
}

void PC13_LED_Flicker(void)
{
	GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_RESET);
	Delay(Flicker_Time);
	GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_SET);
	Delay(Flicker_Time);
}

//OLED回调函数
int I2C_OLED_Display(uint8_t addr, const uint8_t *pdata, uint16_t size)
{
	int ret = I2C_Send_Bytes(g_current_i2c,addr,pdata,size,10);
	return(ret == I2C_SEND_SUCCESS) ? 0 : 1;
}

void init_OLDE(void)
{
	OLED_InitTypeDef OLED_StrucInit = {0};
	OLED_StrucInit.i2c_write_cb = I2C_OLED_Display;
	OLED_Init(&oled,&OLED_StrucInit);
}

void OLED_Show_LED_Status(uint8_t is_on)
{
	OLED_Clear(&oled);              // 清空整个缓冲区

    OLED_SetFont(&oled, &default_font);
    const char *msg = is_on ? "LED: ON" : "LED: OFF";//is_on == 1 send LED: ON
    int16_t sx = (128 - OLED_GetStrWidth(&oled, msg)) / 2;
    // 放在标题下方，标题 y=20, 字体高16, 间距8 → 44
    OLED_SetCursor(&oled, sx, 44);
    OLED_DrawString(&oled, msg);
    OLED_SendBuffer(&oled);
}

void Process_I2C_OLED_Display(void)
{
    //初始化硬件 I2C 和 OLED
	init_I2C1_GPIO();//PB8 PB9
	init_I2C1();
	init_OLDE();

    //设置画笔和画刷（白色字，透明背景）
    OLED_SetPen(&oled, PEN_COLOR_WHITE, 1);
    OLED_SetBrush(&oled, BRUSH_TRANSPARENT);

    OLED_SetFont(&oled, &default_font);
    // 居中标题
    const char *title = "STM32 Ready";
    int16_t tx = (128 - OLED_GetStrWidth(&oled, title)) / 2;
    OLED_SetCursor(&oled, tx, 20);
    OLED_DrawString(&oled, title);


    //把所有绘制内容推送到屏幕
    OLED_SendBuffer(&oled);
}

//配置串口要用的NVIC中断，通过RXNE触发中断
void CE_Init_NVIC_For_USART(void)
{
	// Time clock have open
	NVIC_InitTypeDef NVIC_StructInit = {0};
	NVIC_StructInit.NVIC_IRQChannel = USART1_IRQn;
	NVIC_StructInit.NVIC_IRQChannelPreemptionPriority = 1;//10
	NVIC_StructInit.NVIC_IRQChannelSubPriority = 0;//00
	NVIC_StructInit.NVIC_IRQChannelCmd = ENABLE;

	NVIC_Init(&NVIC_StructInit);
}

//配置按钮要用的EXTI中断，通过检测按键上升沿触发
void CE_Init_EXTI_For_Button(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);//要使用EXTI的线要先开AFIO
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA,GPIO_PinSource1); 
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA,GPIO_PinSource2);

	//配置NVIC
	NVIC_InitTypeDef NVIC_StructInit = {0};

	NVIC_StructInit.NVIC_IRQChannel = EXTI1_IRQn;
	NVIC_StructInit.NVIC_IRQChannelPreemptionPriority = 0;//按钮优先级高于串口
	NVIC_StructInit.NVIC_IRQChannelSubPriority = 0;//按钮1优先级高于按钮2 左大于右
	NVIC_StructInit.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_StructInit);

	NVIC_StructInit.NVIC_IRQChannel = EXTI2_IRQn;
	NVIC_StructInit.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_StructInit.NVIC_IRQChannelSubPriority = 1;
	NVIC_StructInit.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_StructInit);

	//配置EXTI
	EXTI_InitTypeDef EXTI_StructInit = {0};
	EXTI_StructInit.EXTI_Line = EXTI_Line1 | EXTI_Line2;
	EXTI_StructInit.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_StructInit.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_StructInit.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_StructInit);
}

void USART1_IRQHandler(void)
{
	if(USART_GetFlagStatus(USART1,USART_FLAG_RXNE) == SET)
	{
		uint16_t i = USART_ReceiveData(USART1);
		if(i == '1')//串口发 '1' → OLED显示 "LED: ON"，LED常亮 
		{
			led_mode = LED_MODE_ON;
			oled_update = 1;
			oled_display_state = 1;
		}
		else if(i == '0')//串口发 '0' → OLED显示 "LED: OFF"，LED熄灭  
		{
			led_mode = LED_MODE_OFF;
			oled_update = 1;
			oled_display_state = 0;
		}
		else if(i == '2')//串口发 '2' → LED恢复闪烁
		{
			led_mode = LED_MODE_BLINK;
			LED_State = 0;//默认熄灭
			last_toggle_time = GetTick();
		}
		else {}
	}
}

//当前LED状态保存到W25Q64
void EXTI1_IRQHandler(void)
{
	if(EXTI_GetFlagStatus(EXTI_Line1) == SET)
	{
		EXTI_ClearFlag(EXTI_Line1);
		flag_save = 1;
		}
}

//将读出的字节写给PC13
void EXTI2_IRQHandler(void)
{
	if(EXTI_GetFlagStatus(EXTI_Line2) == SET)
	{
		EXTI_ClearFlag(EXTI_Line2);
		flag_load = 1;
	}
}

//保存上次状态，并注入主程序
void Mantain_Last_State(void)
{
	uint8_t saved_state = SPI_W25Q64_Read_Byte();
	if (saved_state == 0x01)
	{
		led_mode = LED_MODE_OFF;   //1 代表熄灭
		GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
		OLED_Show_LED_Status(0);//发送LED OFF
	}
	else if (saved_state == 0x00)
	{
		led_mode = LED_MODE_ON;
		GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);
		OLED_Show_LED_Status(1);//发送LED ON
	}
	else
	{
		led_mode = LED_MODE_BLINK;  // 无效值则默认闪烁
		LED_State = 0;
		last_toggle_time = GetTick();
	}
}

void PROCESS(void)
{
	My_System_Init();//系统时钟初始化 

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//初始化前确定NVIC分组
	
	init_NVIC_Usart_Gpio();//串口_GPIO初始化 PB6 PB7
	init_USART1();
	
	Process_I2C_OLED_Display();//I2C+I2C_GPIO初始化 PB8 PB9   显示：STM32 Ready
	
	init_SPI_GPIO();//SPI_GPIO初始化 PB3/4/5 PA15
	init_SPI();

	init_PC13_GPIO();//初始化板载led PC13
	init_NVIC_Button_1_2_GPIO();//初始化按键1/2 PA1 PA2

	CE_Init_EXTI_For_Button();//初始化按键中断
	CE_Init_NVIC_For_USART();//初始化串口中断


	Mantain_Last_State();

	while(1)
	{
		if(oled_update)//为非0就是真，进if   为0就是假，不进if
		{
			OLED_Show_LED_Status(oled_display_state); // 1-ON  0-OFF  is_on ? "LED: ON" : "LED: OFF"
			oled_update = 0;
		}

		//按下左按键，保存LED状态
		if(flag_save == 1)
		{
			static uint32_t save_debutton_time = 0;
			if(save_debutton_time == 0) save_debutton_time = GetTick();

			if(GetTick() - save_debutton_time >= 20)
			{
				flag_save = 0;
				save_debutton_time = 0;
				
				if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_1) == RESET)
				{
					static uint8_t value_to_save;
					if(led_mode == LED_MODE_ON) value_to_save = 0x00;
					else if(led_mode == LED_MODE_OFF) value_to_save = 0x01;
					else value_to_save = 0xFF;
					SPI_W25Q64_Save_Byte(value_to_save);//使用判断中断标志位，向W25Q64写入，而不是直接获取引脚
				}
			}
		}
		
		//按下右按键，恢复LED状态
		if(flag_load == 1)
		{	
			static uint32_t load_debutton_time = 0;
			if(load_debutton_time == 0) load_debutton_time = GetTick();
			
			if(GetTick() - load_debutton_time >= 20)
			{
				flag_load = 0;	
				load_debutton_time = 0;

				if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_2) == RESET)
				{
					uint8_t i = SPI_W25Q64_Read_Byte();
					if(i == 0x01)
					{
						GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_SET);
						led_mode = LED_MODE_OFF;//恢复上次状态 若收到1 则灯灭 发送OFF
						OLED_Show_LED_Status(0);
					}
					else if(i == 0x00)
					{
						GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_RESET);
						led_mode = LED_MODE_ON;
						OLED_Show_LED_Status(1);
					}
					else//w25q64空时，或者数据不匹配，直接默认闪烁
					{
						led_mode = LED_MODE_BLINK;
						LED_State = 0;//默认置零
						last_toggle_time = GetTick();
					}
				}
			}	
		}

		//LED
		if(led_mode == LED_MODE_ON)//light
		{
			GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_RESET);
		}
		else if(led_mode == LED_MODE_OFF)
		{
			GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_SET);
		}
		else
		{
			if(GetTick() - last_toggle_time >= 1000)//默认闪烁，时间间隔1000ms
			{
				last_toggle_time = GetTick();
				if(LED_State == 0)
				{
					GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_SET);
					LED_State = 1;
				}
				else
				{
					GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_RESET);
					LED_State = 0;
				}
			}
		}
	}

}
