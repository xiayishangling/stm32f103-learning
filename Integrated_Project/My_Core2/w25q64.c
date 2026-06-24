// w25q64.c —— W25Q64 SPI Flash 驱动：
//   vIP_W25Q64Task   ：等待按键队列消息 → 保存或恢复 LED 状态
//   W25Q64_Save_State ：先擦除扇区 → 再写入 1 字节
//   W25Q64_Read_ME   ：读取扇区首字节
//   W25Q64_WaitBusy   ：轮询状态寄存器等待芯片空闲

#include "w25q64.h"
#include "uart_dma.h"

#ifdef W25Q64_MODULE_ENABLED

#define W25Q64_SAVE_KEY_PORT PA3_LED1_GPIO_Port
#define W25Q64_SAVE_KEY_PIN PA3_LED1_Pin

#define W25Q64_CS_PORT PA4_W25Q64_GPIO_Port
#define W25Q64_CS_PIN PA4_W25Q64_Pin
#define W25Q64_HANDLE hspi1

// 等待 KEY_Queue 消息 → 读当前 PA3 电平 → 根据消息类型保存/恢复 LED 状态
// 恢复后直接修改 g_state.led/oled，LED 任务会检测到变化
void vIP_W25Q64Task(void *argument)
{
    /* ---- 上电恢复（只执行一次） ---- */
    Load_Config();

    for(;;)
    {
        KEYMessage msg;
        if(osMessageQueueGet(KEY_QueueHandle, &msg, 0, osWaitForever) == osOK)
        {
            if(msg.w25q64_state == W25Q64_State_Save)
            {
                Save_Config();   // 保存当前全部配置
            }
            else if(msg.w25q64_state == W25Q64_State_Recovery)
            {
                Load_Config();   // 从 Flash 恢复配置
            }
        }
        osDelay(pdMS_TO_TICKS(10));
    }
}

/* 计算 CRC */
static uint32_t Config_CRC(const W25Q64_Config *cfg) 
{
    return (uint32_t)cfg->led_state +
           (uint32_t)(cfg->voltage_threshold * 1000.0f) +
           (uint32_t)(cfg->distance_threshold * 1000.0f);
}

/* 从 Flash 读取配置，校验后应用到全局状态，并打印恢复信息 */
static void Load_Config(void) 
{
    W25Q64_Config cfg;
    W25Q64_Read_All(&cfg);   // 一次读出整个结构体

    uint32_t calc_crc = Config_CRC(&cfg);
    if (cfg.crc == calc_crc) 
    {
        // 数据有效，更新全局状态
        g_state.led = Store_to_LED_State((StoredLedState)cfg.led_state);
        g_state.voltage_threshold = cfg.voltage_threshold;
        g_state.distance_threshold = cfg.distance_threshold;

        UART_DMA_Printf("Restored: LED=%s, Vthr_threshold=%.2fV, Dthr_threshold=%.2fm\r\n",
                        (g_state.led == LED_State_ON) ? "ON" : "OFF",
                        g_state.voltage_threshold,
                        g_state.distance_threshold);
    } 
    else 
    {
        UART_DMA_Printf("Flash config invalid, using defaults.\r\n");
    }
}

/* 从全局状态构造配置并写入 Flash，打印保存信息 */
static void Save_Config(void) 
{
    W25Q64_Config cfg;
    cfg.led_state = (uint8_t)LED_State_to_Store(g_state.led);
    cfg.voltage_threshold = g_state.voltage_threshold;
    cfg.distance_threshold = g_state.distance_threshold;
    cfg.crc = Config_CRC(&cfg);

    W25Q64_Save_All(&cfg);

    UART_DMA_Printf("Saved: LED=%s, Vthr_threshold=%.2fV, Dthr_threshold=%.2fm\r\n",
                    (g_state.led == LED_State_ON) ? "ON" : "OFF",
                    g_state.voltage_threshold,
                    g_state.distance_threshold);
}

// w25q64.c 中增加
bool W25Q64_Erase_Config_Sector(void)
{
    uint8_t cmd;

    // 1. 写使能
    cmd = 0x06;
    HAL_GPIO_WritePin(W25Q64_CS_PORT, W25Q64_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&W25Q64_HANDLE, &cmd, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(W25Q64_CS_PORT, W25Q64_CS_PIN, GPIO_PIN_SET);

    // 2. 扇区擦除命令（0x20），地址 0x000000
    uint8_t erase_cmd[4] = {0x20, 0x00, 0x00, 0x00};
    HAL_GPIO_WritePin(W25Q64_CS_PORT, W25Q64_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&W25Q64_HANDLE, erase_cmd, 4, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(W25Q64_CS_PORT, W25Q64_CS_PIN, GPIO_PIN_SET);

    // 3. 等待擦除完成
    W25Q64_WaitBusy();

    return true;  // 可后续增加状态寄存器检查以确认成功
}

// 保存 结构体到 Flash：写使能 → 擦除扇区0 → 写使能 → 写入
void W25Q64_Save_All(W25Q64_Config *data)
{
    data->crc = data->led_state + (uint32_t)(data->voltage_threshold * 1000)
        + (uint32_t)(data->distance_threshold * 1000);

    uint8_t Buffer[5];
    Buffer[0] = 0x06;//写使能
    HAL_GPIO_WritePin(W25Q64_CS_PORT,W25Q64_CS_PIN ,GPIO_PIN_RESET);
    HAL_SPI_Transmit(&W25Q64_HANDLE,Buffer,1,HAL_MAX_DELAY);
    HAL_GPIO_WritePin(W25Q64_CS_PORT,W25Q64_CS_PIN ,GPIO_PIN_SET);

    Buffer[0] = 0x20;//扇擦除命令
	Buffer[1] = 0x00;
	Buffer[2] = 0x00;
	Buffer[3] = 0x00;
    HAL_GPIO_WritePin(W25Q64_CS_PORT,W25Q64_CS_PIN ,GPIO_PIN_RESET);
    HAL_SPI_Transmit(&W25Q64_HANDLE,Buffer,4,HAL_MAX_DELAY);
    HAL_GPIO_WritePin(W25Q64_CS_PORT,W25Q64_CS_PIN ,GPIO_PIN_SET);
    
    W25Q64_WaitBusy(); 

    Buffer[0] = 0x06;
    HAL_GPIO_WritePin(W25Q64_CS_PORT,W25Q64_CS_PIN ,GPIO_PIN_RESET);
    HAL_SPI_Transmit(&W25Q64_HANDLE,Buffer,1,HAL_MAX_DELAY);
    HAL_GPIO_WritePin(W25Q64_CS_PORT,W25Q64_CS_PIN ,GPIO_PIN_SET);

	Buffer[0] = 0x02;//页编程
	Buffer[1] = 0x00;
	Buffer[2] = 0x00;
	Buffer[3] = 0x00;
    HAL_GPIO_WritePin(W25Q64_CS_PORT,W25Q64_CS_PIN ,GPIO_PIN_RESET);
    HAL_SPI_Transmit(&W25Q64_HANDLE,Buffer,4,HAL_MAX_DELAY);
    HAL_SPI_Transmit(&W25Q64_HANDLE,(uint8_t*)data,sizeof(W25Q64_Config),HAL_MAX_DELAY);
    HAL_GPIO_WritePin(W25Q64_CS_PORT,W25Q64_CS_PIN ,GPIO_PIN_SET);
    
    W25Q64_WaitBusy(); 

    osDelay(pdMS_TO_TICKS(5));
}

// 读取扇区结构体
void W25Q64_Read_All(W25Q64_Config *data)
{
    uint8_t cmd[4] = {0x03,0x00,0x00,0x00};
    W25Q64_WaitBusy(); 
    HAL_GPIO_WritePin(W25Q64_CS_PORT, W25Q64_CS_PIN , GPIO_PIN_RESET);
    HAL_SPI_Transmit(&W25Q64_HANDLE, cmd, 4, HAL_MAX_DELAY);   // 发送读命令
    HAL_SPI_Receive(&W25Q64_HANDLE,(uint8_t*)data, sizeof(W25Q64_Config), HAL_MAX_DELAY);    // HAL会自动发OxFF接收数据
    HAL_GPIO_WritePin(W25Q64_CS_PORT, W25Q64_CS_PIN , GPIO_PIN_SET);
}

// 轮询状态寄存器 BIT0，直到芯片空闲
void W25Q64_WaitBusy(void) 
{
    uint8_t cmd = 0x05, status;
    do {
        HAL_GPIO_WritePin(W25Q64_CS_PORT, W25Q64_CS_PIN , GPIO_PIN_RESET);
        HAL_SPI_Transmit(&W25Q64_HANDLE, &cmd, 1, 100);            // 发送命令
        HAL_SPI_Receive(&W25Q64_HANDLE,&status,1,100);
        HAL_GPIO_WritePin(W25Q64_CS_PORT, W25Q64_CS_PIN , GPIO_PIN_SET);
    } while (status & 0x01);
}

#endif //W25Q64_MODULE_ENABLED

