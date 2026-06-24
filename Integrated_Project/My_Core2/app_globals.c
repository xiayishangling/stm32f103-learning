// app_globals.c —— 全局变量统一定义
// 所有模块通过 app_common.h / app_globals.h 的 extern 声明引用此处的变量

#include "app_common.h"

//————————————无用全局变量 留痕 开始————————————
// volatile LED_State g_state.led = LED_State_OFF;
// W25Q64_State current_w25q64_mode;
// volatile OLED_State g_state.oled = OLED_State_Display3;
//————————————无用全局变量 留痕 结束————————————

// APP全局当前状态初始值，所有任务通过读写 g_state 协同工作
volatile AppState g_state = {
    .led = LED_State_OFF,              // LED 初始关闭
    .oled = OLED_State_Display3,       // OLED 初始显示 "STM32: OK"
    .w25q64 = W25Q64_State_Save,       // W25Q64 初始处于保存模式
    .ps_state = PS_Init,               // 光敏传感器状态机起点
    .ur_state = UR_Send_Pulse_to_Trig, // 超声波状态机起点

    .ps_voltage_v = 0,                  // 光敏电压（V）
    .ur_distance_m = 0,                // 超声波距离（m）

    .voltage_threshold = 1.5f,         // 电压阈值：低于此值 PC13 亮
    .distance_threshold = 0.1f         // 距离阈值：低于此值 PC13 闪烁
};

// 队列消息体——任务间传递用
KEYMessage   keymessage;    // 按键队列消息（→ w25q64 任务）
USARTMessage usartmessage;  // 串口队列消息（→ LED/OLED 任务）
