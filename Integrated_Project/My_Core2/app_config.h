// app_config.h —— 应用模块开关
// 注释掉 #define 即可禁用对应模块，不影响其他模块编译
// （所有跨模块调用的函数在各自 .h 中已用 #else 空宏回退）

#ifndef INC_APP_CONFIG_H_
#define INC_APP_CONFIG_H_

#define APP_COMMON_ENABLED     // 公共类型/枚举（必须开启）

// ----- 模块开关 -----
#define LED_MODULE_ENABLED     // LED（PA3）+ OLED 显示刷新 + PC13 阈值控制 + PA11 闪烁
#define USART_MODULE_ENABLED   // 串口1 接收 + CLI 命令入口
#define KEY_MODULE_ENABLED     // 按键（PA1/PA2）+ EXTI 回调
#define KEY_MODULE2_ENABLED    //状态机消抖回调
#define W25Q64_MODULE_ENABLED  // SPI Flash 保存/恢复 LED 状态
#define OLED_MODULE_ENABLED    // OLED 初始化（I2C）
#define PS_SENSOR_ENABLED      // 光敏传感器（ADC1 注入通道）
#define UR_SENSOR_ENABLED      // 超声波测距（TIM2 输入捕获）
#define CLI_MODULE_ENABLED     // CLI 命令表 + led/dist/lux/help/set
#define DWT_MODULE_ENABLED     // 微秒级延时（DWT->CYCCNT）
//#define CPU_USAGE_ENABLED     // CPU 使用率测量（TIM3）

#endif
