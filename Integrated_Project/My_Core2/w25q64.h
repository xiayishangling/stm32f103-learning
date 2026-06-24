// w25q64.h —— W25Q64 SPI Flash 驱动（保存/恢复 LED 状态）
#ifndef INC_W25Q64_H_
#define INC_W25Q64_H_

#include "app_common.h"
#include "spi.h"

#ifdef W25Q64_MODULE_ENABLED

/* 存入 Flash 的配置结构体 */
typedef struct {
    uint8_t led_state;           // 存储的 LED 状态（见下方枚举）
    float   voltage_threshold;   // 电压阈值（V）
    float   distance_threshold;  // 距离阈值（m）
    uint32_t crc;                // 简易校验和
} W25Q64_Config;

/* 存储用的 LED 状态值 */
typedef enum {
    STORE_LED_OFF = 0x00,
    STORE_LED_ON  = 0x01
} StoredLedState;

/* 从 AppState 的 LED 枚举转到存储值 */
static inline StoredLedState LED_State_to_Store(LED_State state) {
    return (state == LED_State_ON) ? STORE_LED_ON : STORE_LED_OFF;
}

/* 从存储值转到 AppState 的 LED 枚举 */
static inline LED_State Store_to_LED_State(StoredLedState stored) {
    return (stored == STORE_LED_ON) ? LED_State_ON : LED_State_OFF;
}

void    vIP_W25Q64Task(void *argument);
void    W25Q64_Save_All(W25Q64_Config *data);       // 擦除扇区0 并写入 1 字节
void    W25Q64_Read_All(W25Q64_Config *data);       // 读取扇区0 首字节

static uint32_t Config_CRC(const W25Q64_Config *cfg);
static void Load_Config(void);
static void Save_Config(void);
bool W25Q64_Erase_Config_Sector(void);  

void    W25Q64_WaitBusy(void);                         // 等待 Flash 空闲
#else
#define W25Q64_Save_All(data)
#define W25Q64_Read_All(data)
#define Config_CRC(cfg) 
#define Load_Config() 
#define Save_Config()
#define W25Q64_WaitBusy()
#define W25Q64_Erase_Config_Sector()
#endif

#endif
