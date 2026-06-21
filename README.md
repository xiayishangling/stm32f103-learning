# STM32F103C8T6 嵌入式开发学习记录

> 🎬 [演示视频（B站）](https://www.bilibili.com/video/BV1M2j863EG5/)

> **环境**：VSCode + Keil + Keil Assistant → CubeMX + FreeRTOS  
> **库文件**：B站 [铁头山羊](https://space.bilibili.com/471714251) 标准外设库 / HAL 库  
> **方法**：闭卷默写 → 烧录验证 → 错误复盘 → 重构优化

---

## 📁 项目源码

| 项目 | 说明 | 技术栈 |
|------|------|--------|
| `01-BareMetal-Practice/` | 裸机综合练习（GPIO→ADC 全部外设） | 标准外设库 + Keil |
| `02-Timer-Exercises/` | 定时器专项（PWM/输入捕获/主从模式） | 标准外设库 + Keil |
| `Integrated_Project/` | 第一周综合项目 FreeRTOS 重构 | CubeMX + HAL + FreeRTOS |

> `01-BareMetal-Practice/` 和 `02-Timer-Exercises/` 中的 `???` 标记为闭卷默写时的自我纠错注释，记录了从错误到正确的迭代过程。
>
> 标准外设库（`std_periph_driver/`、`startup/`）和 CubeMX 生成文件已通过 `.gitignore` 排除。克隆后需自行添加 ST 标准外设库和 HAL 驱动到对应路径。

---

## 🧠 核心项目架构（Integrated_Project）

```
中断层（极短，只放信号量）
  ├─ USART 接收中断 ──Sem──┐
  └─ KEY 外部中断 ────Sem──┐│
                           ↓↓
任务管道（6 任务，队列/信号量解耦）
  ┌─ vIP_USARTTask ──Queue──→ vIP_LEDTask ───Mutex──→ OLED 刷新
  │   解析串口命令             LED + OLED 控制       （互斥锁保护）
  │
  ├─ vIP_KEYTask ────Queue──→ vIP_W25Q64Task
  │   消抖+识别按键           SPI Flash 保存/恢复
  │
  ├─ vIP_OLEDTask             vIP_PA11Flick
  │   开屏动画                 蓝色 LED 常闪
```

---

## 🔌 硬件接线表（Integrated_Project）

| 外设 | 引脚 | 模式 | 说明 |
|------|------|------|------|
| LED1（红） | PA3 | 推挽输出 | SET=亮 |
| LED2（蓝） | PA11 | 推挽输出 | 常闪 |
| 板载 LED | PC13 | 开漏输出 | 故障指示 |
| 按键1（保存） | PA1 | 上拉输入 | 按下低电平 |
| 按键2（恢复） | PA2 | 上拉输入 | 按下低电平 |
| OLED SCL | PB6 | AF_OD | I2C1 |
| OLED SDA | PB7 | AF_OD | I2C1 |
| W25Q64 CS | PA4 | 推挽输出 | SPI1 片选 |
| W25Q64 SCK | PA5 | AF_PP | SPI1 时钟 |
| W25Q64 MISO | PA6 | IPU | SPI1 输入 |
| W25Q64 MOSI | PA7 | AF_PP | SPI1 输出 |
| USART1 TX | PA9 | AF_PP | 串口发送 |
| USART1 RX | PA10 | IPU | 串口接收 |
| 光敏传感器 AO | PA0 | AIN | ADC1 CH0 |

> `01-BareMetal-Practice/` 和 `02-Timer-Exercises/` 为不同时期的独立项目，引脚分配各有不同，以各自工程内 `main.c` 和 `my_lib/` 中的实际配置为准。

---

## 📖 学习笔记

| 日期 | 主题 | 文档 |
|------|------|------|
| 6.01 | GPIO 按键控制 LED | [docs/6.01-按键控制LED.md](docs/6.01-按键控制LED.md) |
| 6.02 | USART 串口通信 | [docs/6.02-USART串口通信.md](docs/6.02-USART串口通信.md) |
| 6.03 | I2C 通信 | [docs/6.03-I2C通信.md](docs/6.03-I2C通信.md) |
| 6.04 | SPI + W25Q64 Flash | [docs/6.04-SPI通信Flash.md](docs/6.04-SPI通信Flash.md) |
| 6.05 | NVIC + EXTI 中断 | [docs/6.05-中断系统NVIC+EXTI.md](docs/6.05-中断系统NVIC+EXTI.md) |
| 6.06 | 时钟树 + 第一周综合项目 | [docs/6.06-时钟树+综合项目.md](docs/6.06-时钟树+综合项目.md) |
| 6.07 | 定时器 + PWM 呼吸灯 | [docs/6.07-定时器PWM呼吸灯.md](docs/6.07-定时器PWM呼吸灯.md) |
| 6.09 | 输入捕获 + 超声波测距 | [docs/6.09-输入捕获超声波测距.md](docs/6.09-输入捕获超声波测距.md) |
| 6.10 | 主从模式 + PWM 测量 | [docs/6.10-主从模式PWM测量.md](docs/6.10-主从模式PWM测量.md) |
| 6.11 | ADC 光敏传感器 + 回调函数 | [docs/6.11-ADC光敏传感器.md](docs/6.11-ADC光敏传感器.md) |
| 6.14 | ADC 注入序列 + 扫描模式 | [docs/6.14-ADC注入序列+扫描模式.md](docs/6.14-ADC注入序列+扫描模式.md) |
| 6.15 | FreeRTOS 入门 | [docs/6.15-FreeRTOS入门.md](docs/6.15-FreeRTOS入门.md) |
| 6.16 | FreeRTOS 队列实战 | [docs/6.16-FreeRTOS队列实战.md](docs/6.16-FreeRTOS队列实战.md) |
| 6.17 | 二进制信号量 | [docs/6.17-二进制信号量.md](docs/6.17-二进制信号量.md) |
| 6.18 | 综合项目 RTOS 重构 | [docs/6.18-综合项目RTOS重构.md](docs/6.18-综合项目RTOS重构.md) |
| 6.19 | 按键消抖模块化封装 | [docs/6.19-按键消抖模块化.md](docs/6.19-按键消抖模块化.md) |

---

## 📚 心得分享

- [外设中英文对照表](心得分享/外设中英文对照表.md) — 13 大类，180+ 术语

---

## 🛠 外设覆盖

- [x] GPIO（推挽/开漏/上拉/下拉）
- [x] USART（重映射、中断接收、超时保护）
- [x] I2C（主机收发、ACK/NAK、时钟拉伸）
- [x] SPI（全双工、W25Q64 Flash、借时钟）
- [x] NVIC + EXTI（优先级分组、外部中断）
- [x] TIM（时基/PWM/输入捕获/主从模式）
- [x] ADC（常规/注入/扫描/定时器触发）
- [x] OLED（I2C 驱动、回调适配）
- [x] FreeRTOS（任务、队列、信号量、互斥锁）