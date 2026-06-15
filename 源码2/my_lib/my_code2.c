#include "vscode_defs.h"
#include "my_code2.h"

/* ========== 全局变量定义（只在此处定义一次） ========== */

// 系统状态
volatile uint8_t  System_State;
volatile uint8_t  system_err_flag = 0;      // 1-发生错误

// 时间基准
volatile uint32_t currentTick = 0;          // 记录当前时间，单位：ms
volatile uint32_t timer3_overflow_count = 0; // 记录当前时间，单位：us

// LED / 超声波状态
volatile uint8_t  LED_MODE   = LED_OFF;     // 默认熄灭
volatile uint8_t  Rang_State = RANG_NO;
volatile uint8_t  capture_state = 0;

// 状态机1：超声波触发/回波
state_t state = trig_send_begin;

// 输入捕获值
volatile uint16_t ccr1, ccr2;
volatile uint32_t start_time;

// LED 闪烁计时
volatile uint32_t time_led = 0;

// 状态机2：测量PWM
measure_t measure_state = ms_init;
volatile uint32_t ms_time = 0;
volatile uint16_t ms_cc1, ms_cc2;
volatile uint8_t  ms_cc_flag;


void my_one_code2(void)
{
    
}

//纯初始化串口1
void Init_USART1(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO |RCC_APB2Periph_USART1,ENABLE);
	
    GPIO_InitTypeDef GPIO_StructInit = {0};

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

//6.10 测量一个PWM信号 Measure_PWM_signal
//初始化串口的 PB6 TX // PB7 RX
void Init_USART1_Measure_PWM_signal(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO |RCC_APB2Periph_USART1,ENABLE);
	
    GPIO_InitTypeDef GPIO_StructInit = {0};

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

	GPIO_StructInit.GPIO_Pin = GPIO_Pin_13;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_Init(GPIOC,&GPIO_StructInit);

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

//初始化输出比较的 TIM3 - PA6 产生一个PWM信号
void Init_OC_generatePWM_Measure_PWM_signal(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
    GPIO_InitTypeDef GPIO_StructInit = {0};
    GPIO_StructInit.GPIO_Pin = GPIO_Pin_6;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA,&GPIO_StructInit);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE); 

	//初始化TIM3的时基单元
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructInit = {0};
	TIM_TimeBaseStructInit.TIM_Prescaler = 71;
	TIM_TimeBaseStructInit.TIM_Period = 999;
	TIM_TimeBaseStructInit.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructInit.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseStructInit);

	TIM_ARRPreloadConfig(TIM3,ENABLE); //定时器3无RCR
	TIM_Cmd(TIM3,ENABLE);

	//初始化输出比较 定时器3无N
	TIM_OCInitTypeDef TIM_OCStructInit = {0};
	TIM_OCStructInit.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCStructInit.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCStructInit.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCStructInit.TIM_Pulse = 0;
	TIM_OC1Init(TIM3,&TIM_OCStructInit);

	TIM_CtrlPWMOutputs(TIM3,ENABLE);
	TIM_CCPreloadControl(TIM3,ENABLE);//？？？ CCR ARR 都需要开启预加载
}

//初始化输入捕获的 TIM - PA8 CH1 检测TIM3产生的PWM信号
void Init_IC_measurePWM_Measure_PWM_signal(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
    GPIO_InitTypeDef GPIO_StructInit = {0};
    GPIO_StructInit.GPIO_Pin = GPIO_Pin_8;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_IPD;//默认拉低
	GPIO_Init(GPIOA,&GPIO_StructInit);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1,ENABLE); 

	//初始化TIM1的时基单元
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructInit = {0};
	TIM_TimeBaseStructInit.TIM_Prescaler = 71;
	TIM_TimeBaseStructInit.TIM_Period = 65535;
	TIM_TimeBaseStructInit.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructInit.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructInit.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM1,&TIM_TimeBaseStructInit);

	TIM_ARRPreloadConfig(TIM1,ENABLE); 
	TIM_Cmd(TIM1,ENABLE);

	//初始化定时器1的输入捕获
	TIM_ICInitTypeDef TIM_ICStrctInit = {0};
	TIM_ICStrctInit.TIM_Channel = TIM_Channel_1;//CH1
	TIM_ICStrctInit.TIM_ICFilter = 0x0;//消除毛刺
	TIM_ICStrctInit.TIM_ICPolarity = TIM_ICPolarity_Rising;//检测上升沿
	TIM_ICStrctInit.TIM_ICPrescaler = TIM_ICPSC_DIV1;//一分频
	TIM_ICStrctInit.TIM_ICSelection = TIM_ICSelection_DirectTI;//信号选择——直接
	TIM_ICInit(TIM1,&TIM_ICStrctInit);

	TIM_ICStrctInit.TIM_Channel = TIM_Channel_2;
	TIM_ICStrctInit.TIM_ICFilter = 0x0;
	TIM_ICStrctInit.TIM_ICPolarity = TIM_ICPolarity_Falling;
	TIM_ICStrctInit.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICStrctInit.TIM_ICSelection = TIM_ICSelection_IndirectTI;
	TIM_ICInit(TIM1,&TIM_ICStrctInit);

	//初始化定时器1的TRGI 触发输入
	TIM_SelectInputTrigger(TIM1,TIM_TS_TI1FP1);//输入来源 来自 定时器 输入 来自1通道 已滤波 已选择极性 作用于1通道
	TIM_SelectSlaveMode(TIM1,TIM_SlaveMode_Reset);//当 TRGI 引脚检测到上升沿时，自动将计数器 CNT 清零
}

//LED 以sin的周期进行闪烁
void sin_flicker(void)
{
	float t = currentTick * 1.0e-3f;//换算单位，ms->s
	float duty = 0.5f*(sinf((2.0f*3.1416f)*t) + 1);//占空比 上下限0-1,2Π为周期
	uint16_t crr1 = (uint16_t)(duty * 1000.0f);//周期是1000，CCR决定的是高电平所占的比列，CCR = 占空比 * 周期
	TIM_SetCompare1(TIM1,crr1);
}	

// /Trigger CC 中断配置
void TIM1_NVIC_Measure_PWM_signal(void)
{
	TIM_ITConfig(TIM1,TIM_IT_Trigger,ENABLE);
	TIM_ITConfig(TIM1,TIM_IT_CC2,ENABLE);

	NVIC_InitTypeDef NVIC_StructInit = {0};
	NVIC_StructInit.NVIC_IRQChannel = TIM1_CC_IRQn;
	NVIC_StructInit.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_StructInit.NVIC_IRQChannelSubPriority = 0;
	NVIC_StructInit.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_StructInit);

	NVIC_StructInit.NVIC_IRQChannel = TIM1_TRG_COM_IRQn ;
	NVIC_StructInit.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_StructInit.NVIC_IRQChannelSubPriority = 0;
	NVIC_StructInit.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_StructInit);
}

void TIM1_TRG_COM_IRQHandler(void)
{
	if(TIM_GetFlagStatus(TIM1,TIM_FLAG_Trigger) == SET)
	{
        TIM_ClearFlag(TIM1, TIM_FLAG_Trigger);
        ms_cc1 = TIM_GetCapture1(TIM1);   // 立即锁存周期值
        ms_cc_flag |= 0x01;
	}
}

void TIM1_CC_IRQHandler(void)
{
	//测量PWM信号的CC中断
	if(TIM_GetFlagStatus(TIM1,TIM_FLAG_CC2) == SET)
	{
        TIM_ClearFlag(TIM1, TIM_FLAG_CC2);
        ms_cc2 = TIM_GetCapture2(TIM1);   // 立即锁存脉宽
        ms_cc_flag |= 0x02;
	}

	//超声波部分CC中断
	if(TIM_GetFlagStatus(TIM1,TIM_FLAG_CC1) == SET)
	{
		TIM_ClearFlag(TIM1,TIM_FLAG_CC1);
		capture_state |= 0x01;
	}
	if(TIM_GetFlagStatus(TIM1,TIM_FLAG_CC2) == SET)
	{
		TIM_ClearFlag(TIM1,TIM_FLAG_CC2);
		capture_state |= 0x02;
	}
	if(capture_state == 0x03)
	{
		Rang_State = RANG_OK;
		capture_state = 0;
	}
}

//测量一个PWM信号的状态机
void Measure_PWM_signal_state_machine(void)
{
	switch (measure_state)
	{
	case ms_init:
		{
			ms_cc_flag = 0x00;
			TIM_ClearFlag(TIM1,TIM_FLAG_Trigger);
			TIM_ClearFlag(TIM1,TIM_FLAG_CC1 | TIM_FLAG_CC2);//清除所有可能意外标志位

			TIM_SetCompare1(TIM3,200);

			ms_time = currentTick;
			measure_state = wait_trigger;
			break;
		}
	case wait_trigger:
		{
			if(ms_cc_flag & 0x01)
			{
				measure_state = ms_process;
			}
			else if(currentTick - ms_time > 20)
			{
				ms_time = 0;
				measure_state = inter_val;
			}
			break;
		}
	case ms_process:
		{
			// 等待下降沿捕获完成（CC2 标志置位） CC1 此时已经获取了上一个最高点，即整个周期，CC2还没检测到
    		if (ms_cc_flag == 0x03)
			{
				TIM_ClearFlag(TIM1, TIM_FLAG_CC2);
				LED_MODE = LED_ON;
				time_led = 0; 

				float period = ms_cc1 * 1.0e-6f * 1.0e3f;//周期 ms
				float duty = ((float)ms_cc2) / ms_cc1 * 100.0f;//占空比
				char buff[64];
				sprintf(buff,"Period = %.3f  duty = %.3f %%\r\n",period,duty);
				send_a_characterstring_usart(USART1,(uint8_t *)buff,strlen(buff),20);//0-255m

				ms_time = currentTick;
				measure_state = inter_val;
			}
			break;
		}
	case inter_val:
		{
			if(ms_time == 0) ms_time = currentTick;
			if(currentTick - ms_time > 1000)
			{
				LED_MODE = LED_OFF;
				ms_cc_flag = 0x00;
				measure_state = ms_init;
				//此处可用标志位返回 yes or no
			}	
			break;
		}
	}
}

//操作 测量一个PWM信号
void Process_Measure_PWM_signal(void)
{
	Init_USART1_Measure_PWM_signal();
	Init_OC_generatePWM_Measure_PWM_signal();
	Init_IC_measurePWM_Measure_PWM_signal();
	Init_TIM2();
	while(1)
	{
		//sin_flicker(); //测试初始化OC是否成功

		Measure_PWM_signal_state_machine();
		LED_state_machine();
	}
}


//6.9 超声波测距部分
//us级计时器初始化
void us_TIM(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);

	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructInit = {0};
	TIM_TimeBaseStructInit.TIM_Prescaler = 71;
	TIM_TimeBaseStructInit.TIM_Period = 0xFFFF;//65535 约65ms溢出一次
	TIM_TimeBaseStructInit.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructInit.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseStructInit);

	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);

	//nvic
	NVIC_InitTypeDef NVIC_StructInit = {0};
	NVIC_StructInit.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_StructInit.NVIC_IRQChannelPreemptionPriority = 3;//低于输入捕获 低于ms计数器 最低
	NVIC_StructInit.NVIC_IRQChannelSubPriority = 0;
	NVIC_StructInit.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_StructInit);

	TIM_Cmd(TIM3,ENABLE);
}

//us级计时器中断
void TIM3_IRQHandler(void)
{
	if(TIM_GetFlagStatus(TIM3,TIM_FLAG_Update) == SET)
	{
		TIM_ClearFlag(TIM3,TIM_FLAG_Update);
		timer3_overflow_count++;//65ms溢出一次，count记一次
	}
}

//us级计时器 获取当前总微秒数
uint32_t micros(void)
{
	static uint32_t ovf,cnt;
	do{
		ovf = timer3_overflow_count;//获取当前溢出了几次 通过<<16 移动到了高16位
		cnt = TIM3->CNT;//获取剩余数 低16位
	}while(ovf != timer3_overflow_count);//防止读取ovf刚好溢出，导致ovf不匹配
	return ((ovf << 16) | cnt);//65536 == 2^16  ovf<<16 == ovf*16 // |上cnt，拼接成完整32位
}

//us级计时器 阻塞式微秒级延时
void My_Delay_2_us(uint32_t us)
{
	uint32_t start = micros();
	while((micros() - start) < us);
}

//初始化超声波测距的时基单元与输入捕获
void Init_Timebase_InputCapture(void)
{
	//初始化时基单元
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1,ENABLE);

	TIM_TimeBaseInitTypeDef TIM_StructInit = {0};
	TIM_StructInit.TIM_Prescaler = 71;//PSC
	TIM_StructInit.TIM_Period = 65535;//ARR
	TIM_StructInit.TIM_CounterMode = TIM_CounterMode_Up;//Counter state-up / down / center
	TIM_StructInit.TIM_RepetitionCounter = 0;//RCR
	TIM_TimeBaseInit(TIM1,&TIM_StructInit);

	TIM_ITConfig(TIM1,TIM_IT_CC1,ENABLE);
	TIM_ITConfig(TIM1,TIM_IT_CC2,ENABLE);

	NVIC_InitTypeDef NVIC_StructInit = {0};
	NVIC_StructInit.NVIC_IRQChannel = TIM1_CC_IRQn;
	NVIC_StructInit.NVIC_IRQChannelPreemptionPriority = 1;//低于ms计数器，高于us计数器
	NVIC_StructInit.NVIC_IRQChannelSubPriority = 0;
	NVIC_StructInit.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_StructInit);

	TIM_ICInitTypeDef TIM_ICStrctInit = {0};
	TIM_ICStrctInit.TIM_Channel = TIM_Channel_1;//CH1
	TIM_ICStrctInit.TIM_ICFilter = 0x0;//消除毛刺
	TIM_ICStrctInit.TIM_ICPolarity = TIM_ICPolarity_Rising;//检测上升沿
	TIM_ICStrctInit.TIM_ICPrescaler = TIM_ICPSC_DIV1;//一分频
	TIM_ICStrctInit.TIM_ICSelection = TIM_ICSelection_DirectTI;//信号选择——直接
	TIM_ICInit(TIM1,&TIM_ICStrctInit);

	TIM_ICStrctInit.TIM_Channel = TIM_Channel_2;
	TIM_ICStrctInit.TIM_ICFilter = 0x0;
	TIM_ICStrctInit.TIM_ICPolarity = TIM_ICPolarity_Falling;
	TIM_ICStrctInit.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICStrctInit.TIM_ICSelection = TIM_ICSelection_IndirectTI;
	TIM_ICInit(TIM1,&TIM_ICStrctInit);

	TIM_Cmd(TIM1,ENABLE);
}

//超声波测距状态机
void ultrasonic_ranging_state_machine(void)
{
		switch (state)
		{
		case trig_send_begin:
			{		
				//1.清空CNT
				TIM_SetCounter(TIM1,0);
				//2.清空cc1 cc2
				TIM_ClearFlag(TIM1,TIM_FLAG_CC1 | TIM_FLAG_CC2);
				Rang_State = RANG_NO;//清空上次状态
				capture_state = 0x00;//每次发送清空中断标志位 

				TIM_ITConfig(TIM1, TIM_IT_CC1 | TIM_IT_CC2, ENABLE);//防御性编程，防止其它地方关闭
				TIM_Cmd(TIM1, ENABLE); 

				//3.发送10us的脉冲
				GPIO_WriteBit(GPIOA,GPIO_Pin_1,Bit_SET);//out_od -默认拉低
				My_Delay_2_us(10);
				GPIO_WriteBit(GPIOA,GPIO_Pin_1,Bit_RESET);

				start_time = currentTick;
				state = wait_echo;
				break;
			}
		case wait_echo:
			{
				if(Rang_State == RANG_OK)
				{
					state = process;
					break;
				}
				else if(currentTick - start_time > 30)
				{
					Rang_State = RANG_NO;
					state = interval;
				}
				break;
			}	

		case process:
			{
				ccr1 = TIM_GetCapture1(TIM1);
				ccr2 = TIM_GetCapture2(TIM1);
				uint32_t width = (ccr2 >= ccr1) ? (ccr2 - ccr1) : ((0xFFFF - ccr1) + ccr2 +1);//us
				if(width > 100 && width < 20000)//有效距离约为： 1.7 cm ~ 3.4 m
				{
					LED_MODE = LED_ON;
					time_led = 0;  

					float distance = (width*1.0e-6f*340.0f)/2.0f;
					char buff[32];
					sprintf(buff,"%.3f m\r\n",distance);
					send_a_characterstring_usart(USART1,(uint8_t *)buff,strlen(buff),20);//0-255m
				}
				start_time = currentTick;
				state = interval;
				break;
			}
		
		case interval:
			{
				TIM_Cmd(TIM1, DISABLE); 
				if(currentTick - start_time > 2000)//延时2s,每隔2s测一次距离
				{
					state = trig_send_begin;
					Rang_State = RANG_NO;
				}
				break;
			}
		}
}

//操作超声波测距
void Process_ultrasonic_ranging(void)
{
	//初始化板载ledPC13；trig PA1-推挽通用输出 // Echo CH1 PA8-输入下拉 // 串口GPIO:PB6 TX/PB7 RX //  串口初始化
	Init_GPIO_USART_For_ultrasonic_ranging();
	us_TIM();//us count
	Init_TIM2();//ms count 
	Init_Timebase_InputCapture();
	
	while (1)
	{
		ultrasonic_ranging_state_machine();
		LED_state_machine();
	}
	
}

//LED状态机，每次测距板载LED亮0.5s
void LED_state_machine(void)
{
	if(LED_MODE == LED_ON)
	{
		if(time_led == 0) time_led = currentTick;
		GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_RESET);
		if(currentTick - time_led > 500)
		{
			LED_MODE = LED_OFF;
			time_led = 0;
		}
	}
	else
	{
		GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_SET);
	}
}


//6.7呼吸灯部分
//初始化 TIM1 的 CHx CHnx  CH1-PA8  CH1N-PB13  在输出比较模式下，都设置成复用推挽
void init_CCR1_Compare_GPIO(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB,ENABLE);//位掩码 0000 1100
	GPIO_InitTypeDef GPIO_StructInit = {0};
	GPIO_StructInit.GPIO_Pin = GPIO_Pin_13;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB,&GPIO_StructInit);

	GPIO_StructInit.GPIO_Pin = GPIO_Pin_8;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA,&GPIO_StructInit);
}

void init_CCR1_Compare(void)
{
	//确认定时时间为1ms 周期为1000
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1,ENABLE);
	TIM_TimeBaseInitTypeDef TIM_StructInit = {0};
	TIM_StructInit.TIM_Prescaler = 71;//1MHz PSC
	TIM_StructInit.TIM_Period = 999;//1ms ARR
	TIM_StructInit.TIM_CounterMode = TIM_CounterMode_Up;//up
	TIM_StructInit.TIM_RepetitionCounter = 0;// one RCR
	TIM_TimeBaseInit(TIM1,&TIM_StructInit);

	TIM_ARRPreloadConfig(TIM1,ENABLE);//ARR CCR 的影子寄存器都是默认未开启；PSC RCR是默认关闭的
	TIM_Cmd(TIM1,ENABLE);

	TIM_CtrlPWMOutputs(TIM1,ENABLE);//MOE ENABLE

	TIM_OCInitTypeDef TIM_OC_StructInit = {0};
	TIM_OC_StructInit.TIM_OCMode = TIM_OCMode_PWM1;//8种模式的选择
	TIM_OC_StructInit.TIM_OCPolarity = TIM_OCPolarity_High;//指定正常输出通道的输出极性
	TIM_OC_StructInit.TIM_OCNPolarity = TIM_OCPolarity_High;//指定互补输出通道的输出极性 -不经过反相器
	TIM_OC_StructInit.TIM_OutputNState = TIM_OutputNState_Enable;//指定定时器互补输出比较通道的状态（使能/禁用）CH1N
	TIM_OC_StructInit.TIM_OutputState = TIM_OutputState_Enable;//指定定时器输出比较通道的状态（使能/禁用） CH1
	TIM_OC_StructInit.TIM_Pulse = 0;//指定要加载到捕获/比较寄存器中的脉冲值 初始值
	TIM_OC_StructInit.TIM_OCIdleState = TIM_OCIdleState_Reset;//定时器停止工作时，正常输出引脚（CHx）会强制变成的电平状态
	TIM_OC_StructInit.TIM_OCNIdleState = TIM_OCNIdleState_Reset;//定时器停止工作时，互补输出引脚（CHxN）会强制变成的电平状态
	TIM_OC1Init(TIM1,&TIM_OC_StructInit);
}

void Process_Breathing_lamp(void)
{
	init_CCR1_Compare_GPIO();
	init_CCR1_Compare();
	Init_TIM2();

	while(1)
	{
		float t = currentTick * 1.0e-3f;//换算单位，ms->s
		float duty = 0.5f*(sinf((2.0f*3.1416f)*t) + 1);//占空比 上下限0-1,2Π为周期
		uint16_t crr1 = (uint16_t)(duty * 1000.0f);//周期是1000，CCR决定的是高电平所占的比列，CCR = 占空比 * 周期
		TIM_SetCompare1(TIM1,crr1);
	}
}


//6.7定时器部分
void init_PC13_GPIO(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);
	GPIO_InitTypeDef GPIO_StructInit = {0};
	GPIO_StructInit.GPIO_Pin = GPIO_Pin_13;
	GPIO_StructInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_StructInit.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_Init(GPIOC,&GPIO_StructInit);
}

void PC13_LED_Flicker(void)
{
	GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_RESET);
	My_Delay_1(1000);
	GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_SET);
	My_Delay_1(1000);
}

void My_Delay_1(uint32_t ms)
{
	uint32_t expireTick = currentTick + ms;
	while(currentTick < expireTick);
}

/*// PSC 71+1 -- 1MHz == 1us  //  周期 = ARR + 1  //  RCR = 0 -- 产生updata = 0 + 1
//1us * 1000 = 1ms
//updata产生一次，发生一次中断，1ms，current +1 ，相当于计时 1ms  相当于GetTick（）——获取当前时间*/
void Init_TIM2(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);//走PCLK 一个外设，时钟默认关闭

	TIM_TimeBaseInitTypeDef TIM_StructInit = {0};
	TIM_StructInit.TIM_Prescaler = 71;//PSC
	TIM_StructInit.TIM_Period = 999;//ARR
	TIM_StructInit.TIM_CounterMode = TIM_CounterMode_Up;//Counter state-up / down / center
	TIM_StructInit.TIM_RepetitionCounter = 0;//RCR
	TIM_TimeBaseInit(TIM2,&TIM_StructInit);
	//TIM_ARRPreloadConfig(TIM2,ENABLE);//??? 我认为为了防止ARR被突然修改，需要开启影子寄存器  结论：没必要
	TIM_Cmd(TIM2,ENABLE);

	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);

	NVIC_InitTypeDef NVIC_StructInit = {0};//在内核中，不需要开时钟，默认开启
	NVIC_StructInit.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_StructInit.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_StructInit.NVIC_IRQChannelSubPriority = 0;
	NVIC_StructInit.NVIC_IRQChannelCmd = ENABLE;

	NVIC_Init(&NVIC_StructInit);
}

void TIM2_IRQHandler(void)
{
	if(TIM_GetFlagStatus(TIM2,TIM_FLAG_Update) == SET)
	{
		TIM_ClearFlag(TIM2,TIM_FLAG_Update);//??? 之前忘记清除标志位了
		currentTick++;
	}
}

void Process_mydelay(void)
{
	init_PC13_GPIO();
	Init_TIM2();
	while (1)
	{
		//阻塞设计
		//PC13_LED_Flicker();

		//非阻塞延时
		static uint32_t last_blink_tick = 0;
		static uint8_t led_flag_tick = 0;
		
		if(LED_MODE == LED_ON)
		{
			GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_SET);
			if(!led_flag_tick)// 0 真，进入if 获取时间
			{
				last_blink_tick = currentTick;
				led_flag_tick = 1;//获取时间后置1，等led变化再置0，下次可重新获取时间
			}	 
			if(currentTick - last_blink_tick >= 1000)
			{
				LED_MODE = LED_OFF;
				led_flag_tick = 0;
			}
		}
		else
		{
			GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_RESET);
			if(!led_flag_tick)// 0 真，进入if 获取时间
			{
				last_blink_tick = currentTick;
				led_flag_tick = 1;
			}	
			if(currentTick - last_blink_tick >= 1000)
			{
				LED_MODE = LED_ON;
				led_flag_tick = 0;
			}
		}	
	}
	
}


//6.7时钟部分
//未注释掉了starup中的系统自带System Init函数  ————用于练习标志位
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
		system_err_flag = 1;
		System_State = SYSTEM_HSE_TIMEOUT;
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
		system_err_flag = 1;
		System_State = SYSTEM_PLL_TIMEOUT;
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
		//while(1);//死锁，放弃治疗，等看门狗重置整个系统

		system_err_flag = 1;
		System_State = SYSTEM_SELECT_ERR;
		//返回
		//跳出
	}
	
	if(system_err_flag == 0) return INIT_SYSTEM_SUCCESS;
	else return INIT_SYSTEM_ERR;
}

//此函数只做练习 留痕 观看 学习标志位的使用 未写全
//核心要点是，要么跳出，要么返回，简略练习也要注意最基本逻辑，至少需要标清楚
void Process_systeminit(void)
{
	My_System_Init();

	while(1)
	{
		if(system_err_flag == 1)
		{
			if(System_State == SYSTEM_HSE_TIMEOUT)
			{
				//操作
				system_err_flag = 0;
			}
			else if(System_State == SYSTEM_PLL_TIMEOUT)
			{
				//操作
				system_err_flag = 0;
			}
			else
			{
				static uint32_t system_select_timeout = 0;
				uint32_t timeout = 0x0500;//10个周期，1-16us，等待起振
				// 切回 HSI
				RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
				if(system_select_timeout == 0) system_select_timeout = GetTick();
				while(RCC_GetSYSCLKSource() != 0x00)
				{
					if(GetTick() - system_select_timeout >= timeout)
					{
						//返回错误
						//跳出
						system_select_timeout = 0;
					}
				}
				SystemCoreClock = 8000000;   // 更新为 HSI 频率

				RCC_HCLKConfig(RCC_HCLK_Div1);
				RCC_PCLK1Config(RCC_HCLK_Div2);
				RCC_PCLK2Config(RCC_HCLK_Div1);

				RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
				while(RCC_GetSYSCLKSource() != 0x00 && (--timeout))//练习，用两种方式判断超时
				if(timeout == 0)
				{
					//返回结果
					//操作
					//跳出
				}

				system_err_flag = 0;

				//返回结果
			}
		}
	}
}


