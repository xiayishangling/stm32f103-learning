// key_task.c —— 按键驱动：
//   HAL_GPIO_EXTI_Callback → 释放信号量 → vIP_KEYTask 消抖 → 发送队列消息
//   同时包含按键消抖状态机（key_idle → press_debounce → pressed → release_debounce）

#include "key_task.h"
#include "uart_dma.h"

#ifdef KEY_MODULE_ENABLED

// PA1/PA2 按键中断 → 释放二进制信号量通知 KEY 任务
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if((GPIO_Pin == PA1_KEY1_Pin) || (GPIO_Pin == PA2_KEY2_Pin))
    { 
        osSemaphoreRelease(KEY_BinarySemHandle);
    }
}

// 等待按键信号量 → 20ms 消抖 → 读电平判断 KEY1/KEY2 → 发送 W25Q64 状态到队列
void vIP_KEYTask(void *argument)
{
    for(;;)
    {
        KEYMessage msg;
        osSemaphoreAcquire(KEY_BinarySemHandle,osWaitForever);
        vTaskDelay(pdMS_TO_TICKS(20));
        if(HAL_GPIO_ReadPin(PA1_KEY1_GPIO_Port,PA1_KEY1_Pin) == GPIO_PIN_RESET)
        {
            msg.w25q64_state = W25Q64_State_Save;
        }
        else if(HAL_GPIO_ReadPin(PA2_KEY2_GPIO_Port,PA2_KEY2_Pin) == GPIO_PIN_RESET)
        {
            msg.w25q64_state = W25Q64_State_Recovery;
        }
        else
            continue;

        if(osMessageQueuePut(KEY_QueueHandle,&msg,0,0) != osOK)
        {
            UART_DMA_Printf("Queue_Full\r\n");
        }
    }
}

#endif //KEY_MODULE_ENABLED



#ifdef KEY_MODULE2_ENABLED

#define KEY_DEBOUCNE_TIME_MS 20

//初始化按键1实例
key_debounce_t key1 = {
    .port = GPIOA,
    .pin = GPIO_PIN_1,
    .key_debounce_state = key_idle,
    .key_debounce_tick = 0,
    .on_press = NULL
};

//初始化按键2实例
key_debounce_t key2 = {
    .port = GPIOA,
    .pin = GPIO_PIN_2,
    .key_debounce_state = key_idle,
    .key_debounce_tick = 0,
    .on_press = NULL
};

// 四态消抖状态机：idle → press_debounce → pressed → release_debounce
// 按下/释放均需稳定 20ms 才确认，通过回调 on_press 通知应用层
void KEY_DeBounce_State_Machine(key_debounce_t *key)
{
    uint8_t key_level = HAL_GPIO_ReadPin(key->port,key->pin);

    switch (key->key_debounce_state)
    {
    case key_idle:
        {
            if(key_level == GPIO_PIN_RESET)
            {
                key->key_debounce_tick = osKernelGetTickCount();//rate_hz == 1000 一个系统节拍1ms
                key->key_debounce_state = key_press_debounce;
            }
            break;
        }
    case key_press_debounce:
        {
            if(key_level != GPIO_PIN_RESET)
            {
                key->key_debounce_state = key_idle;
            }
            else
            {
                if(osKernelGetTickCount() - key->key_debounce_tick > KEY_DEBOUCNE_TIME_MS)
                {
                    key->key_debounce_state = key_pressed;
                    if(key->on_press) key->on_press();                
                }
            }
            break;
        }
    case key_pressed:
        {
            if(key_level != GPIO_PIN_RESET)
            {
                key->key_debounce_tick = osKernelGetTickCount();
                key->key_debounce_state = key_release_debounce;
            }
            break;
        }
    case key_release_debounce:
        {
            if(key_level == GPIO_PIN_RESET)
            {
                key->key_debounce_state = key_pressed;
            }
            else if(key_level != GPIO_PIN_RESET)
            {
                if(osKernelGetTickCount() - key->key_debounce_tick > KEY_DEBOUCNE_TIME_MS)
                {
                    key->key_debounce_state = key_idle;
                }
            }
            break;      
        }
    }
}

void KEY_SetPressCallback(key_debounce_t *key,key_callback_t callback)
{
    key->on_press = callback;
}

//使用按键回调时 放到最上面初始化
void KEY_DeBounce_Init(void)
{
    KEY_SetPressCallback(&key1,KEY1_Callback);
    KEY_SetPressCallback(&key2,KEY2_Callback);
}

//回调函数 按下按钮发生的操作
void KEY1_Callback(void)
{
    // 示例 如传递keymessage
    // KEYMessage msg = {.w25q64_state = W25Q64_Save_State};
    // osMessageQueuePut(KEY_QueueHandle,&msg,0,0);
}

void KEY2_Callback(void)
{
    // 示例 如传递keymessage
    // KEYMessage msg = {.w25q64_state = W25Q64_State_Recovery};
    // osMessageQueuePut(KEY_QueueHandle,&msg,0,0);
}

#endif //KEY_MODULE2_ENABLED


