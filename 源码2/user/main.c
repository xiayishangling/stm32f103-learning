#include "vscode_defs.h"  // 先加宏定义，再包含其他头文件
#include "stm32f10x.h"
#include "my_code.h"
#include "my_code2.h"
#include "delay.h"
#include "oled.h"
#include <stdio.h>
#include "math.h"
#include "string.h"
#include "usart.h"

#define DEBOUNCE_MS 20   // 消抖时间 20ms

//PTS_SS 标志位部分 , 使用枚举类型定义一系列标志位
typedef enum{PS_OK,PS_NO,PS_EMPTY}pst_ss_state;
pst_ss_state  PS_STATE;
volatile uint32_t PS_time_start;

//PST_SS 状态机部分
typedef enum{ps_init,ps_send_pulse,ps_wait_conv_ok,ps_process,ps_interval,ps_restart}pst_ss_state_machine;
pst_ss_state_machine ps_state_machine = ps_init;

//LED 标志位部分
typedef enum{LED_ON_1,LED_OFF_1,LED_EMPTY_1,LED_FLICKER}led_state;
led_state LED_STATE = LED_EMPTY_1;

//LED 闪烁 标志位
typedef enum{LED_FLICKER_ON,LED_FLICKER_OFF,LED_FLICKER_EMPTY}led_flicker_state;
led_flicker_state ledf_state = LED_FLICKER_EMPTY;

//按键回调函数
typedef void(*key_callback_t)(void);

//按键消抖状态机
typedef enum
{
	key_idle,			//按键空闲，等待按下
	press_debounce,		//按下消抖中
	pressed,			//确认按下（稳定）
	release_debounce	//释放消抖
}key_debounce_state_machine;//按键状态机

typedef struct {
	key_debounce_state_machine key_state;//按键状态机进程
	uint32_t key_debounce_tick;//按键状态机时间
	uint8_t press_flag;//确认按下时置1
	key_callback_t on_press; //按下回调（确认按下时调用）
	key_callback_t on_press_IS;//注入序列按键按下  IS:Injected Sequence
	key_callback_t on_press_SP;//扫描模式按键
} key_debounce_t;//按键状态机封装 进程 时间

// 全局定义按键实例
key_debounce_t key1;   // PA2 按键
key_debounce_t key2;   // PA3 按键
key_debounce_t key3;   // PA4 按键

volatile uint32_t Tick = 0;//全局时间计时

//6.14 扫描模式
void Init_ADC_scan_patterns(void);//初始化扫描模式的PA0 PA1
void key_callback_SP(void);//SP scan patterns
void Key_Init_SP(void);//初始化 扫描模式 按键消抖
void ADC_scan_patterns_state_machine(void);//扫描模式状态机
void Process_ADC_scan_patterns(void);//操作扫描模式

//6.14 定时器触发注入序列
void Key_Init_IS(void);//初始化 注入序列 按键消抖
void key_callback_IS(void);
void Init_TIM_Injected_Sequence(void);
void Process_TIM_Injected_Sequence(void);
void photosensitive_sensor_state_machine_IS(void);
void Init_TIM_Injected_Sequence_IT(void);
void ADC1_2_IRQHandler(void);

//6.11 ADC 光敏传感器实验
void Init_photosensitive_sensor(void);
void photosensitive_sensor_state_machine(void);
void Process_photosensitive_sensor(void);

void LED_1_state_machine(void);//按键状态机 ON-点亮  OFF-熄灭
void Key_Debounce_StateMachine(GPIO_TypeDef *GPIOx,uint16_t GPIO_Pin,key_debounce_t *key);//按键消抖状态机
void Key_SetPressCallback(key_debounce_t *key,key_callback_t callback);
void key_callback_restart(void);

int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	//Process_mydelay();
	//Process_Breathing_lamp();
	//Process_ultrasonic_ranging();
	//Process_Measure_PWM_signal();
	//Process_photosensitive_sensor();
	//Process_TIM_Injected_Sequence();
	Process_ADC_scan_patterns();
	while(1)
	{
	}
}

//按键消抖状态机
void Key_Debounce_StateMachine(GPIO_TypeDef *GPIOx,uint16_t GPIO_Pin,key_debounce_t *key)
{
	uint8_t key_level = GPIO_ReadInputDataBit(GPIOx,GPIO_Pin);

	switch (key->key_state)
	{
	case key_idle://按键空闲
		{
			if(key_level == Bit_RESET)
			{	
				key->key_debounce_tick = currentTick;
				key->key_state = press_debounce;	
			}
			break;
		}
		
	case press_debounce://按下消抖
		{
			if(key_level != Bit_RESET)
			{
				key->key_state = key_idle;
			}
			else
			{
				if(currentTick - key->key_debounce_tick > DEBOUNCE_MS)
				{
					key->press_flag = 1;
					key->key_state = pressed;
					if(key->on_press != NULL)
					{
						key->on_press();
						key->on_press_IS();
						key->on_press_SP();
					}
				}
			}
			break;
		}

	case pressed://确认按下
		{
			if(key_level != Bit_RESET)//松开？ 
			{
				key->key_debounce_tick = currentTick;
				key->key_state = release_debounce;
			}
			break;
		}

	case release_debounce://释放消抖
		{
			if(key_level == Bit_RESET)
			{
				key->key_state = pressed;
			}
			else if(key_level != Bit_RESET)
			{
				if(currentTick - key->key_debounce_tick > DEBOUNCE_MS)
				{
					key->key_state = key_idle;
				}
			}
			break;
		}
	}
	
}

//按键消抖回调函数
void Key_SetPressCallback(key_debounce_t *key,key_callback_t callback)
{
	key->on_press = callback;
	key->on_press_IS = callback;
	key->on_press_SP = callback;
}

//LED状态机
void LED_1_state_machine(void)
{
	if(LED_STATE == LED_ON_1)
	{
		GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);
	}
	else if(LED_STATE == LED_OFF_1)
	{
		GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
	}
	else if(LED_STATE == LED_EMPTY_1)
	{
		//功能待补充
		//其它？
	}
	else if(LED_STATE == LED_FLICKER)
	{
		if(currentTick - Tick > 500)
		{
			Tick = currentTick;
			if(ledf_state == LED_FLICKER_OFF)
			{
				GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
				ledf_state = LED_FLICKER_ON;
			}
			else if(ledf_state == LED_FLICKER_ON)
			{
				GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);
				ledf_state = LED_FLICKER_OFF;
			}
		}

	}
}

//JEOC中断
void ADC1_2_IRQHandler(void)
{
	if(ADC_GetFlagStatus(ADC1,ADC_FLAG_JEOC) == SET)
	{
		ADC_ClearFlag(ADC1,ADC_FLAG_JEOC);
		PS_STATE = PS_OK;
	}
}

//6.14 扫描模式
void Key_Init_SP(void)
{
	key3.key_state = key_idle;
    key3.key_debounce_tick = 0;
	key3.press_flag = 0;
	key3.on_press_IS = NULL;

	Key_SetPressCallback(&key3,key_callback_SP);
}

//按下PA4按键的回调 对其进行的操作 5s后重启
void key_callback_SP(void)
{
	PS_time_start = currentTick;
	ps_state_machine = ps_restart;
}

//初始化扫描模式的有关参数
void Init_ADC_scan_patterns(void)
{
	//初始化PA4按键
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
    GPIO_InitTypeDef GPIO_StructInit = {0};
    GPIO_StructInit.GPIO_Pin = GPIO_Pin_4;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA,&GPIO_StructInit);

	//PA0 PA1 模拟输入
	GPIO_StructInit.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA,&GPIO_StructInit);


	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1,ENABLE); 

	//初始化TIM1的时基单元
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructInit = {0};
	TIM_TimeBaseStructInit.TIM_Prescaler = 71;
	TIM_TimeBaseStructInit.TIM_Period = 999;
	TIM_TimeBaseStructInit.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructInit.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructInit.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM1,&TIM_TimeBaseStructInit);

	TIM_SelectOutputTrigger(TIM1,TIM_TRGOSource_Update);
	TIM_Cmd(TIM1,ENABLE);

	//初始化ADC时钟 分频数/开时钟
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);//12MHz
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);

	//初始化ADC部分参数
	ADC_InitTypeDef ADC_StructInit = {0};
	ADC_StructInit.ADC_Mode = ADC_Mode_Independent;//ADC1 独立工作
	ADC_StructInit.ADC_ContinuousConvMode = DISABLE;//不开启连续模式
	ADC_StructInit.ADC_DataAlign = ADC_DataAlign_Right;//数据右对齐
	ADC_StructInit.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;//soft
	ADC_StructInit.ADC_NbrOfChannel = 2;//注入序列2个通道
	ADC_StructInit.ADC_ScanConvMode = ENABLE;//开启扫描
	ADC_Init(ADC1,&ADC_StructInit);

	ADC_InjectedSequencerLengthConfig(ADC1,2);
	ADC_ExternalTrigInjectedConvConfig(ADC1,ADC_ExternalTrigInjecConv_T1_TRGO);//定时器1的TRGO
	ADC_ExternalTrigInjectedConvCmd(ADC1,ENABLE);
	ADC_InjectedChannelConfig(ADC1,ADC_Channel_0,1,ADC_SampleTime_13Cycles5);
	ADC_InjectedChannelConfig(ADC1,ADC_Channel_1,2,ADC_SampleTime_13Cycles5);

	ADC_Cmd(ADC1, ENABLE);
	ADC_ResetCalibration(ADC1);//校准定时器
	while (ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1));
	ADC_Cmd(ADC1, DISABLE);//等待状态机开启关闭
}

//扫描模式状态机 ————未新建新枚举 用的是“定时器触发注入序列”改出来的
void ADC_scan_patterns_state_machine(void)
{
	switch (ps_state_machine)//调整两个滑动变阻器，控制led的状态
	{
	case ps_init:
		{
			ADC_ClearFlag(ADC1,ADC_FLAG_JEOC);
			ADC_Cmd(ADC1,DISABLE);
			PS_time_start = 0;
			PS_STATE = PS_EMPTY;

			ps_state_machine = ps_send_pulse;
			break;
		}
	
	case ps_send_pulse:
		{
			ADC_Cmd(ADC1,ENABLE);//打开开关 自动每1ms 通过TRGO 发送一次脉冲

			PS_time_start = currentTick;
			ps_state_machine = ps_wait_conv_ok;
			break;
		}

	case ps_wait_conv_ok:
		{
			if(PS_STATE == PS_OK)
			{
				PS_time_start = currentTick;
				ps_state_machine = ps_process;
				PS_STATE = PS_NO;
			}
			if(currentTick - PS_time_start > 20)//20ms 标志位/中断未触发 进入超时
			{
				PS_time_start = 0;
				ps_state_machine = ps_interval;
			}
			break;
		}

	case ps_process:
		{
			uint16_t jdr1 = ADC_GetInjectedConversionValue(ADC1,ADC_InjectedChannel_1);//读取JDR1通道的数据
			uint16_t jdr2 = ADC_GetInjectedConversionValue(ADC1,ADC_InjectedChannel_2);//读取JDR2通道的数据
			ADC_Cmd(ADC1,DISABLE);//暂时关闭
			float v1 = jdr1 * (3.3f / 4095);//把结果转换成电压 2^12 12逐渐逼近型ADC
			float v2 = jdr2 * (3.3f / 4095);
			printf("%.3f,%.3f\n",v1,v2);//向串口发送每次电压

			if((v1 > 1.5) && (v2 > 1.5))
			{
				LED_STATE = LED_ON_1;//开灯
				
			}
			else if((v1 < 1.5) && (v2 < 1.5))
			{
				if(LED_STATE != LED_FLICKER)
				{
					Tick = currentTick;
					ledf_state = LED_FLICKER_OFF;
				}
				LED_STATE = LED_FLICKER;//闪烁
			}
			else
			{
				LED_STATE = LED_OFF_1;//关灯
			}

			ps_state_machine = ps_interval;
			break;
		}

	case ps_interval:
		{
			if(PS_time_start == 0 )
			{
				ps_state_machine = ps_init;//重启程序
				//返回超时
			}
			ADC_Cmd(ADC1,ENABLE);//再次开启
			PS_time_start = currentTick;
			ps_state_machine = ps_wait_conv_ok;
			break;
		}
		
	case ps_restart://按下PA4 按钮 重启程序
		{
			if(currentTick - PS_time_start > 5000)//等待5s重启程序
			{
				ps_state_machine = ps_init;
			}
			break;
		}
	}
}

//操作扫描模式
void Process_ADC_scan_patterns(void)
{
	Init_TIM2();//启动ms定时器
	Init_USART1();//初始化USART1
	init_PC13_GPIO();//初始化板载LED 低电平点亮 PC13
	Key_Init_SP();
	Init_ADC_scan_patterns();
	Init_TIM_Injected_Sequence_IT();//开启JEOC中断

	while(1)
	{
		ADC_scan_patterns_state_machine();
		LED_1_state_machine();
		Key_Debounce_StateMachine(GPIOA,GPIO_Pin_4,&key3);
	}
}



//6.14 定时器触发注入序列
void Key_Init_IS(void)
{
    key2.key_state = key_idle;
    key2.key_debounce_tick = 0;
	key2.press_flag = 0;
	key2.on_press_IS = NULL;

	Key_SetPressCallback(&key2,key_callback_IS);
}

//按下PA3按键的回调 对其进行的操作 5s后重启整个系统
void key_callback_IS(void)
{
	PS_time_start = currentTick;
	ps_state_machine = ps_restart;
}

//初始化注入序列 等基本参数配置
void Init_TIM_Injected_Sequence(void)
{	
	//初始化PA3按键
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
    GPIO_InitTypeDef GPIO_StructInit = {0};
    GPIO_StructInit.GPIO_Pin = GPIO_Pin_3;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA,&GPIO_StructInit);

	//PA0 模拟输入
	GPIO_StructInit.GPIO_Pin = GPIO_Pin_0;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA,&GPIO_StructInit);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1,ENABLE); 

	//初始化TIM1的时基单元
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructInit = {0};
	TIM_TimeBaseStructInit.TIM_Prescaler = 71;
	TIM_TimeBaseStructInit.TIM_Period = 999;
	TIM_TimeBaseStructInit.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructInit.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructInit.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM1,&TIM_TimeBaseStructInit);

	TIM_SelectOutputTrigger(TIM1,TIM_TRGOSource_Update);
	TIM_Cmd(TIM1,ENABLE);

	//初始化ADC时钟 分频数/开时钟
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);//12MHz
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);

	//初始化ADC部分参数
	ADC_InitTypeDef ADC_StructInit = {0};
	ADC_StructInit.ADC_Mode = ADC_Mode_Independent;//ADC1 独立工作
	ADC_StructInit.ADC_ContinuousConvMode = DISABLE;//不开启连续模式
	ADC_StructInit.ADC_DataAlign = ADC_DataAlign_Right;//数据右对齐
	ADC_StructInit.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;//soft
	ADC_StructInit.ADC_NbrOfChannel = 1;//注入序列一个通道
	ADC_StructInit.ADC_ScanConvMode = DISABLE;//不开启扫描
	ADC_Init(ADC1,&ADC_StructInit);

	ADC_InjectedSequencerLengthConfig(ADC1,1);
	ADC_ExternalTrigInjectedConvConfig(ADC1,ADC_ExternalTrigInjecConv_T1_TRGO);//定时器1的TRGO
	ADC_ExternalTrigInjectedConvCmd(ADC1,ENABLE);
	ADC_InjectedChannelConfig(ADC1,ADC_Channel_0,1,ADC_SampleTime_13Cycles5);

	ADC_Cmd(ADC1, ENABLE);
	ADC_ResetCalibration(ADC1);//校准定时器
	while (ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1));
	ADC_Cmd(ADC1, DISABLE);//等待状态机开启关闭
}

//配置注入序列 中断
void Init_TIM_Injected_Sequence_IT(void)
{
	ADC_ITConfig(ADC1,ADC_IT_JEOC,ENABLE);

	NVIC_InitTypeDef NVIC_StructInit = {0};
	NVIC_StructInit.NVIC_IRQChannel = ADC1_2_IRQn;
	NVIC_StructInit.NVIC_IRQChannelPreemptionPriority = 2;//低于ms计数器
	NVIC_StructInit.NVIC_IRQChannelSubPriority = 0;
	NVIC_StructInit.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_StructInit);
}



//光敏传感器状态机 注入序列版本
void photosensitive_sensor_state_machine_IS(void)
{
	switch (ps_state_machine)
	{
	case ps_init:
		{
			ADC_ClearFlag(ADC1,ADC_FLAG_JEOC);
			ADC_Cmd(ADC1,DISABLE);
			PS_time_start = 0;
			PS_STATE = PS_EMPTY;

			ps_state_machine = ps_send_pulse;
			break;
		}
	
	case ps_send_pulse:
		{
			ADC_Cmd(ADC1,ENABLE);//打开开关 自动每1ms 通过TRGO 发送一次脉冲

			PS_time_start = currentTick;
			ps_state_machine = ps_wait_conv_ok;
			break;
		}

	case ps_wait_conv_ok:
		{
			if(PS_STATE == PS_OK)
			{
				PS_time_start = currentTick;
				ps_state_machine = ps_process;
				PS_STATE = PS_NO;
			}
			if(currentTick - PS_time_start > 20)//20ms 标志位/中断未触发 进入超时
			{
				PS_time_start = 0;
				ps_state_machine = ps_interval;
			}
			break;
		}

	case ps_process:
		{
			uint16_t jdr = ADC_GetInjectedConversionValue(ADC1,ADC_InjectedChannel_1);//读取JDR1通道的数据
			ADC_Cmd(ADC1,DISABLE);//暂时关闭
			float voltage = jdr * (3.3f / 4095);//把结果转换成电压 2^12 12逐渐逼近型ADC
			printf("%.3f\n",voltage);//向串口发送每次电压

			if(voltage > 1.5)//外界光线亮  电阻小 分压少 AO降低
			{
				LED_STATE = LED_ON_1;//太亮关灯
			}
			else
			{
				LED_STATE = LED_OFF_1;//太暗开灯
			}

			ps_state_machine = ps_interval;
			break;
		}

	case ps_interval:
		{
			if(PS_time_start == 0 )
			{
				ps_state_machine = ps_init;//重启程序
				//返回超时
			}
			ADC_Cmd(ADC1,ENABLE);//再次开启
			PS_time_start = currentTick;
			ps_state_machine = ps_wait_conv_ok;
			break;
		}
		
	case ps_restart://按下PA3 按钮 重启程序
		{
			if(currentTick - PS_time_start > 5000)//等待5s重启程序
			{
				ps_state_machine = ps_init;
			}
			break;
		}
	}
}

//操作注入序列 光敏传感器 状态机
void Process_TIM_Injected_Sequence()
{
	Init_TIM2();//启动ms定时器
	Init_USART1();//初始化USART1
	init_PC13_GPIO();//初始化板载LED 低电平点亮 PC13
	Init_TIM_Injected_Sequence();
	Init_TIM_Injected_Sequence_IT();
	Key_Init_IS();

	while(1)
	{
		photosensitive_sensor_state_machine_IS();
		LED_1_state_machine();
		Key_Debounce_StateMachine(GPIOA,GPIO_Pin_3,&key2);
	}
}



//6.11 ADC 光敏传感器实验
//初始化按键消抖
void Key_Init(void)
{
    key1.key_state = key_idle;
    key1.key_debounce_tick = 0;
	key1.press_flag = 0;
	key1.on_press = NULL;

	Key_SetPressCallback(&key1,key_callback_restart);
}

//按下按钮后的操作 回调 5s后重启整个系统
void key_callback_restart(void)
{
	PS_time_start = currentTick;
	ps_state_machine = ps_restart;
}

//初始化光敏传感器有关参数
void Init_photosensitive_sensor(void)
{
	// init PA0 PC13
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC,ENABLE);
    GPIO_InitTypeDef GPIO_StructInit = {0};
    GPIO_StructInit.GPIO_Pin = GPIO_Pin_0;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_AIN;//模拟信号输入
	GPIO_Init(GPIOA,&GPIO_StructInit);

	GPIO_StructInit.GPIO_Pin = GPIO_Pin_13;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_Init(GPIOC,&GPIO_StructInit);

	GPIO_StructInit.GPIO_Pin = GPIO_Pin_2;//按键PA2
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA,&GPIO_StructInit);

	//初始化ADC时钟 分频数/开时钟
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);//12MHz
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);

	//初始化ADC部分参数
	ADC_InitTypeDef ADC_StructInit = {0};
	ADC_StructInit.ADC_Mode = ADC_Mode_Independent;//ADC1 独立工作
	ADC_StructInit.ADC_ContinuousConvMode = DISABLE;//不开启连续模式
	ADC_StructInit.ADC_DataAlign = ADC_DataAlign_Right;//数据右对齐
	ADC_StructInit.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;//soft
	ADC_StructInit.ADC_NbrOfChannel = 1;//常规序列1个通道
	ADC_StructInit.ADC_ScanConvMode = DISABLE;//不开启扫描
	ADC_Init(ADC1,&ADC_StructInit);

	//配置常规序列
	ADC_RegularChannelConfig(ADC1,ADC_Channel_0,1,ADC_SampleTime_13Cycles5);
	ADC_ExternalTrigConvCmd(ADC1,ENABLE);
}

//光敏传感器状态机
void photosensitive_sensor_state_machine(void)
{
	switch (ps_state_machine)
	{
	case ps_init:
		{
			ADC_ClearFlag(ADC1,ADC_FLAG_EOC);
			ADC_Cmd(ADC1,DISABLE);
			PS_time_start = 0;
			PS_STATE = PS_EMPTY;//PS_OK NO 是为了中断设计的 此处还未用到

			ps_state_machine = ps_send_pulse;
			break;
		}
	
	case ps_send_pulse:
		{
			ADC_Cmd(ADC1,ENABLE);
			ADC_ClearFlag(ADC1,ADC_FLAG_EOC);
			ADC_SoftwareStartConvCmd(ADC1,ENABLE);//软件启动的方式发送脉冲

			PS_time_start = currentTick;
			ps_state_machine = ps_wait_conv_ok;
			break;
		}

	case ps_wait_conv_ok:
		{
			if(ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC) == SET)
			{
				PS_time_start = currentTick;
				ps_state_machine = ps_process;
			}
			if(currentTick - PS_time_start > 30)
			{
				PS_time_start = 0;
				ps_state_machine = ps_interval;
			}
			break;
		}

	case ps_process:
		{
			uint16_t dr = ADC_GetConversionValue(ADC1);//读取转换的结果
			float voltage = dr * (3.3f / 4095);//把结果转换成电压 2^12 12逐渐逼近型ADC
			printf("ADC: %d, Voltage: %.3f V\r\n", dr, voltage);//向串口发送每次电压

			if(voltage > 1.5)//外界光线亮  电阻小 分压少 AO降低
			{
				LED_STATE = LED_ON_1;//太亮关灯
			}
			else
			{
				LED_STATE = LED_OFF_1;//太暗开灯
			}

			PS_time_start = currentTick;
			ps_state_machine = ps_interval;
			break;
		}

	case ps_interval:
		{
			if(PS_time_start == 0 )
			{
				PS_time_start = currentTick;
				//返回超时
			}
			if(currentTick - PS_time_start > 1000)//每隔一秒进行一次检测
			{
				ps_state_machine = ps_init;
			}
			break;
		}
		
	case ps_restart:
		{
			if(currentTick - PS_time_start > 5000)//等待5s重启程序
			{
				ps_state_machine = ps_init;
			}
		}
	}
}



//操作 光敏传感器的函数
void Process_photosensitive_sensor(void)
{
	Init_photosensitive_sensor();
	Init_TIM2();//启动ms计数器，currentTick
	Key_Init();
	Init_USART1();

	while (1)
	{
		photosensitive_sensor_state_machine();
		//LED_state_machine();//LED状态机，state = LED_ON时 led亮0.5s 只有一个led，暂时用不到
		Key_Debounce_StateMachine(GPIOA, GPIO_Pin_2, &key1);
		LED_1_state_machine();
	}
	
}

