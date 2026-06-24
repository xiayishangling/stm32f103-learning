// cli.c —— CLI 命令解析：
//   CLI_Command 结构体 = 命令名 + 帮助文本 + 函数指针
//   cmd_table[] 命令表存放所有命令条目（{NULL,NULL,NULL} 收尾）
//   CLI_Parse() 按空格切字符串 → 查表 → 调用 handler(argc, argv)
//   添加新命令：写一个 cmd_xxx() + 在 cmd_table 里加一行即可
#include "cli.h"
#include "uart_dma.h"
#include "w25q64.h"

#ifdef CLI_MODULE_ENABLED

//CLI模板部分
/* 前置声明——函数体在文件末尾 */
/* 格式: static void cmd_xxx(int argc, char *argv[]) */
static void cmd_led (int argc, char *argv[]);
static void cmd_dist(int argc, char *argv[]);
static void cmd_lux (int argc, char *argv[]);
static void cmd_help(int argc, char *argv[]);
static void cmd_set(int argc, char *argv[]);
static void cmd_w25q64(int argc, char *argv[]);

/* 命令表：每个条目 = {命令名, 帮助文本, 处理函数}
   末尾 {NULL,NULL,NULL} 是查表终点，必须保留 */
static const CLI_Command cmd_table[] = {
    {"led",  "led <on|off|flicker>  - control LED",     cmd_led },
    {"dist", "dist                  - show distance",    cmd_dist},
    {"lux",  "lux                   - show light intensity", cmd_lux },
    {"help", "help                  - list all commands",cmd_help},
    {"set", "set vthr/dthr <value>  - contron threshold", cmd_set},
    {"w25q64","w25q64 clear         - Erase the flash chip",cmd_w25q64},
    {NULL, NULL, NULL}   // 结束标记
};

/* ========== CLI_Parse：按空格切分 → 查表 → 调用 ========== */
void CLI_Parse(char *line)
{
    int argc = 0;
    char *argv[CLI_MAX_ARGS] = {0};

    /* 1. 按空格切分字符串 */
    char *token = strtok(line, " \t");
    while (token != NULL && argc < CLI_MAX_ARGS) {
        argv[argc++] = token;
        token = strtok(NULL, " \t");
    }
    if (argc == 0) return;   // 空行，忽略

    /* 2. 查命令表 */
    for (const CLI_Command *cmd = cmd_table; cmd->name != NULL; cmd++) {
        if (strcmp(argv[0], cmd->name) == 0) {
            cmd->handler(argc, argv);   // 找到，调用
            return;
        }
    }

    /* 3. 未找到 */
    UART_DMA_Printf("Unknown command: %s\r\nType 'help' to list commands.\r\n", argv[0]);
}

/* ========== 命令实现 ========== */

// led <on|off|flicker>
static void cmd_led(int argc, char *argv[])
{
    if (argc < 2) {
        UART_DMA_Printf("Usage: led <on|off|flicker>\r\n");
        return;
    }

    USARTMessage msg;
    memset(&msg, 0, sizeof(msg));

    if (strcmp(argv[1], "on") == 0) {
        msg.led_state  = LED_State_ON;
        msg.oled_state = OLED_State_Display1;
    } else if (strcmp(argv[1], "off") == 0) {
        msg.led_state  = LED_State_OFF;
        msg.oled_state = OLED_State_Display2;
    } else if (strcmp(argv[1], "flicker") == 0) {
        msg.led_state  = LED_State_Flicker;
        msg.oled_state = OLED_State_Display3;
    } else {
        UART_DMA_Printf("Unknown arg: %s. Usage: led <on|off|flicker>\r\n", argv[1]);
        return;
    }

    if (osMessageQueuePut(USART_QueueHandle, &msg, 0, pdMS_TO_TICKS(100)) != osOK) {
        UART_DMA_Printf("usartQueue_err\r\n");
    }
}

// dist - 显示超声波距离
static void cmd_dist(int argc, char *argv[])
{
    g_state.oled = OLED_State_Display_distance;
    UART_DMA_Printf("Distance: %.3f m\r\n", g_state.ur_distance_m);
}

// lux - 显示光敏电压
static void cmd_lux(int argc, char *argv[])
{
    g_state.oled = OLED_State_Display_voltage;
    UART_DMA_Printf("Light voltage: %.3f V\r\n", g_state.ps_voltage_v);
}

// set vthr xxx   dthr xxx  控制阈值
static void cmd_set(int argc, char *argv[])
{
    if (argc < 3) {
        UART_DMA_Printf("Usage: set vthr <value> | dthr <value>\r\n");
        return;
    }
    if (strcmp(argv[1], "vthr") == 0) {
        g_state.voltage_threshold = atof(argv[2]);
        UART_DMA_Printf("Voltage threshold set to %.2f V\r\n", g_state.voltage_threshold);
    } else if (strcmp(argv[1], "dthr") == 0) {
        g_state.distance_threshold = atof(argv[2]);
        UART_DMA_Printf("Distance threshold set to %.2f m\r\n", g_state.distance_threshold);
    } else {
        UART_DMA_Printf("Unknown parameter\r\n");
    }
}

static void cmd_w25q64(int argc, char *argv[])
{
    if (argc < 2) {
        UART_DMA_Printf("Usage: w25q64 clear\r\n");
        return;
    }

    if (strcmp(argv[1], "clear") == 0) {
        // 执行擦除
        if (W25Q64_Erase_Config_Sector()) {
            // 重置全局状态为默认值（与 app_globals.c 中的初始值保持一致）
            g_state.led = LED_State_OFF;
            g_state.oled = OLED_State_Display3;
            g_state.voltage_threshold = 1.5f;
            g_state.distance_threshold = 0.1f;
            // 如果有其他从 Flash 恢复的字段也一并重置

            UART_DMA_Printf("Configuration erased and defaults restored.\r\n");
        } else {
            UART_DMA_Printf("Erase failed.\r\n");
        }
    } else {
        UART_DMA_Printf("Unknown subcommand. Use 'w25q64 clear'.\r\n");
    }
}

// help - 列出所有命令
static void cmd_help(int argc, char *argv[])
{
    printf("\r\n===== Commands =====\r\n");
    for (const CLI_Command *cmd = cmd_table; cmd->name; cmd++)
        printf("  %s\r\n", cmd->help);
}

#endif // CLI_MODULE_ENABLED




