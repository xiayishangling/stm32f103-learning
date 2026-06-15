# STM32F103C8T6 学习笔记

从零开始学习 STM32 裸机开发，使用 **VSCode + Keil + Keil Assistant**，每天闭卷默写代码 + 烧录验证。

## 学习路线

| 日期 | 主题 | 文档 |
|------|------|------|
| 6.1 | GPIO 按键控制 LED | [docs/6.01-按键控制LED.md](docs/6.01-按键控制LED.md) |
| 6.2 | USART 串口通信 | [docs/6.02-USART串口通信.md](docs/6.02-USART串口通信.md) |
| 6.3 | I2C + SPI 通信 | [docs/6.02-USART串口通信.md](docs/6.02-USART串口通信.md) |
| ... | ... | ... |

## 源码

- `源码1/` — Practice 基础练习工程
- `源码2/` — Timer 定时器工程

> 使用 B站 [铁头山羊](https://space.bilibili.com/471714251) 的库文件

> ⚠️ 每日练习代码分散在 `源码1/user/main.c` 和 `源码1/my_lib/my_code.c / my_code2.c` 中，函数声明较乱，后续会统一整理。

## 外设覆盖

- [x] GPIO（推挽/开漏/上拉/下拉）
- [x] USART（重映射、中断接收）
- [x] I2C（主机收发、ACK/NACK）
- [x] SPI（W25Q64 Flash 驱动）
- [x] NVIC + EXTI（中断优先级、外部中断）
- [x] TIM（时基/PWM/输入捕获/主从触发）
- [x] ADC（常规/注入/扫描/定时器触发）
