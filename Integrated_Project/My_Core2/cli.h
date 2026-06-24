// cli.h —— CLI 命令解析器
// CLI_Command 结构体: name + help + handler
// CLI_Parse: 空格切字符串 → 查 cmd_table → 调用对应函数
#ifndef INC_CLI_H_
#define INC_CLI_H_

#include "app_common.h"

#ifdef CLI_MODULE_ENABLED

#define CLI_CMD_BUF_SIZE 32  // 命令缓冲区大小
#define CLI_MAX_ARGS     4   // 最多参数个数

typedef struct {
    const char *name;                          // 命令名，如 "led"
    const char *help;                           // 帮助文本
    void (*handler)(int argc, char *argv[]);    // 处理函数指针
} CLI_Command;

void CLI_Parse(char *line);                    // 解析一行命令输入

#else
#define CLI_Parse(line)
#endif

#endif
