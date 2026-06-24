// key_task.h —— 按键驱动 + 消抖状态机
#ifndef INC_KEY_TASK_H_
#define INC_KEY_TASK_H_

#include "app_common.h"

#ifdef KEY_MODULE_ENABLED

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);               // EXTI 中断回调
void vIP_KEYTask(void *argument);                              // 按键任务（等待信号量）

#endif //KEY_MODULE_ENABLED



#ifdef KEY_MODULE2_ENABLED

typedef void (*key_callback_t)(void);  // 按键回调函数类型

// 按键消抖实例（每个按键一个）
typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
    enum { key_idle, key_press_debounce, key_pressed, key_release_debounce } key_debounce_state;
    uint32_t      key_debounce_tick;
    key_callback_t on_press;           // 按下确认后的回调
} key_debounce_t;

void KEY_DeBounce_State_Machine(key_debounce_t *key);          // 消抖状态机
void KEY_DeBounce_Init(void);                                  // 初始化按键回调
void KEY_SetPressCallback(key_debounce_t *key, key_callback_t callback);
void KEY1_Callback(void);                                      // KEY1 按下回调
void KEY2_Callback(void);                                      // KEY2 按下回调

#else
#define KEY_DeBounce_State_Machine(key)
#define KEY_DeBounce_Init()

#endif //KEY_MODULE2_ENABLED


#endif //INC_KEY_TASK_H_
