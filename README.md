# STM32F103C8T6 嵌入式开发实践

> 🎬 [演示视频（B站）](https://www.bilibili.com/video/BV1RFjo64Ew1/)

> **环境**：VSCode + Keil + Keil Assistant → CubeMX + FreeRTOS  
> **库文件**：B站 [铁头山羊](https://space.bilibili.com/471714251) 标准外设库 / HAL 库  
> **方法**：闭卷默写 → 烧录验证 → 错误复盘 → 重构优化

---

## ⏳ 学习时间线

- **大二**：通过学校作业（循迹小车、阿里云智慧家居）接触嵌入式开发，熟悉了传感器驱动、蓝牙通信、云平台对接等基本概念
- **2025 下半年**：系统学习 STM32，参与机器狗实训，自学 C 语言和标准外设库
- **2026 上半年**：持续深入外设驱动、中断系统、RTOS 等，积累底层驱动代码
- **2026.06**：开始用 Git 统一管理，将之前的代码和项目独立重新实现，逐日提交至本仓库


---

## 📁 项目源码

| 项目 | 说明 | 技术栈 |
|------|------|--------|
| `01-BareMetal-Practice/` | 裸机综合实践（GPIO→ADC 全部外设） | 标准外设库 + Keil |
| `02-Timer-Exercises/` | 定时器专项（PWM/输入捕获/主从模式） | 标准外设库 + Keil |
| `Integrated_Project/` | 综合项目 FreeRTOS 重构 | CubeMX + HAL + FreeRTOS |

---

## 🚀 快速开始

1. `git clone` 本仓库，补充 ST 标准外设库和 HAL 驱动到 `Drivers/` 路径
2. 使用 Keil MDK 打开对应项目下的 `.uvprojx`，编译烧录到 STM32F103C8T6 最小系统板
3. 串口连接 PA9/PA10，115200 波特率，输入 `help` 查看 CLI 命令

> `Integrated_Project.ioc` 为 CubeMX 工程文件，可双击打开查看时钟树（HSE 8MHz → PLL 72MHz）、外设引脚配置、FreeRTOS 参数等。CubeMX 生成代码已通过 `.gitignore` 排除。


> `01-BareMetal-Practice/` 和 `02-Timer-Exercises/` 中的 `???` 标记为独立实现时的自我纠错注释，记录了从错误到正确的迭代过程。
>
> 标准外设库（`std_periph_driver/`、`startup/`）和 CubeMX 生成文件已通过 `.gitignore` 排除。克隆后需自行添加 ST 标准外设库和 HAL 驱动到对应路径。

---

## 🧠 核心项目架构（Integrated_Project）

### 文件组织

```
Integrated_Project/
├── Core/                    # CubeMX 生成（HAL 驱动、中断、FreeRTOSConfig）
│   ├── Inc/                 #   外设头文件（gpio/tim/usart/...）
│   └── Src/                 #   外设初始化 + main.c + freertos.c
├── Drivers/                 # STM32 HAL 库 + CMSIS（gitignore）
├── Middlewares/             # FreeRTOS 源码（gitignore）
├── MDK-ARM/                 # Keil 工程 + 编译产物（gitignore）
├── My_Core2/                # ★ 用户代码核心 ★
│   ├── app_config.h         #   模块开关（一行注释即可禁用模块）
│   ├── app_common.h         #   公共类型/枚举/队列/状态机 + AppState
│   ├── app_globals.c/h      #   全局变量定义
│   ├── key_task.c/h         #   按键 EXT I + 状态机消抖
│   ├── led_task.c/h         #   LED 控制 + OLED 刷新 + 阈值指示灯
│   ├── usart_task.c/h       #   串口接收 → CLI 命令入口
│   ├── uart_dma.c/h         #   UART1 DMA 打印（带互斥锁 printf 替代）
│   ├── cli.c/h              #   CLI 命令解析器（led/dist/lux/help/set/w25q64）
│   ├── w25q64.c/h           #   SPI Flash 保存/恢复（阈值持久化）
│   ├── oled_task.c/h        #   OLED 初始化 + 状态显示
│   ├── ps_sensor.c/h        #   光敏传感器（ADC1 注入 + TIM1 TRGO）
│   ├── ur_sensor.c/h        #   超声波测距（TIM2 输入捕获 CH1/CH2）
│   └── dwt.c/h              #   DWT 微秒级延时
└── OLED_Driver/             # OLED 底层驱动（I2C）
```

### 模块开关系统

所有模块通过 `app_config.h` 统一控制，禁用模块后对应函数自动变为空宏，**无需修改业务代码**：

```c
// app_config.h
#define LED_MODULE_ENABLED      // 开
//#define KEY_MODULE2_ENABLED    // 关 → 所有 KEY_DeBounce 调用自动变空
```

```c
// 各 .h 中的模式
#ifdef MODULE_ENABLED
    void Func(void);            // 真实函数
#else
    #define Func()              // 空宏，调用处安全消失
#endif
```

### 数据流 & 任务架构

```
 ┌─────────────── 信号量层（ISR → 任务）───────────────┐
 │                                                      │
 │  USART RX ──CountingSem──→ vIP_USARTTask              │
 │  KEY EXTI ──BinarySem────→ vIP_KEYTask                │
 │  ADC1 JEOC──BinarySem────→ vIP_PS_IJ_Task             │
 │  TIM2 CC1/2─BinarySem────→ vIP_UR_Task                │
 └──────────────────────────────────────────────────────┘
                         │
         ┌───────────────┼───────────────┐
         ↓               ↓               ↓
    ┌─────────┐    ┌──────────┐    ┌──────────┐
    │ CLI 层   │    │ 队列层    │    │ 直接控制  │
    │          │    │          │    │          │
    │ led on   │    │ USART    │    │ vIP_LED  │
    │ led off  │    │  Queue   │    │  Task    │
    │ lux  │    │   ↓      │    │          │
    │ dist  │    │ LED      │    │ vIP_PC13 │
    │ help     │    │  Task    │    │  Start   │
    │          │    │          │    │  Task    │
    └─────────┘    │ KEY      │    │          │
                   │  Queue   │    │ vIP_PA11 │
                   │   ↓      │    │  Flick   │
                   │ W25Q64   │    │          │
                   │  Task    │    └──────────┘
                   └──────────┘
```

### CLI 命令表

| 命令 | 参数 | 功能 |
|------|------|------|
| `led` | `on` / `off` / `flicker` | 控制 PA3 LED |
| `dist` | — | 显示当前超声波距离 |
| `lux` | — | 显示当前光敏电压 |
| `set` | `vthr <V>` / `dthr <m>` | 设置电压/距离阈值 |
| `w25q64` | `clear` | 擦除 Flash 并恢复默认值 |
| `help` | — | 列出所有命令 |

---

## 🔌 硬件接线表（Integrated_Project）

| 外设 | 引脚 | 模式 | 说明 |
|------|------|------|------|
| LED（红） | PA3 | 推挽输出 | SET=亮 |
| LED（蓝） | PA11 | 推挽输出 | 心跳闪烁（500ms） |
| 板载 LED | PC13 | 开漏输出 | 阈值指示灯 |
| 按键1（保存） | PA1 | 上拉输入 | 保存阈值到 Flash |
| 按键2（恢复） | PA2 | 上拉输入 | 从 Flash 恢复阈值 |
| 按键3 | PB0 | 上拉输入 | 备用按键 |
| OLED SCL | PB6 | AF_OD | I2C1 |
| OLED SDA | PB7 | AF_OD | I2C1 |
| W25Q64 CS | PA4 | 推挽输出 | SPI1 片选 |
| W25Q64 SCK | PA5 | AF_PP | SPI1 时钟 |
| W25Q64 MISO | PA6 | IPU | SPI1 输入 |
| W25Q64 MOSI | PA7 | AF_PP | SPI1 输出 |
| USART1 TX | PA9 | AF_PP | 串口发送 + DMA |
| USART1 RX | PA10 | IPU | 串口接收 + DMA |
| 光敏传感器 AO | PA0 | AIN | ADC1 注入通道 |
| 超声波 Trig | PB14 | 推挽输出 | 触发脉冲 |
| 超声波 Echo | PA15 | IPU | TIM2 CH1 输入捕获（重映射） |

---


---

## 📖 学习笔记

| # | 主题 | 文档 |
|---|------|------|
| 01 | GPIO 按键控制 LED | [docs/01-按键控制LED.md](docs/01-按键控制LED.md) |
| 02 | USART 串口通信 | [docs/02-USART串口通信.md](docs/02-USART串口通信.md) |
| 03 | I2C 通信 | [docs/03-I2C通信.md](docs/03-I2C通信.md) |
| 04 | SPI + W25Q64 Flash | [docs/04-SPI通信Flash.md](docs/04-SPI通信Flash.md) |
| 05 | NVIC + EXTI 中断 | [docs/05-中断系统NVIC+EXTI.md](docs/05-中断系统NVIC+EXTI.md) |
| 06 | 时钟树 + 第一周综合项目 | [docs/06-时钟树+综合项目.md](docs/06-时钟树+综合项目.md) |
| 07 | 定时器 + PWM 呼吸灯 | [docs/07-定时器PWM呼吸灯.md](docs/07-定时器PWM呼吸灯.md) |
| 08 | 输入捕获 + 超声波测距 | [docs/09-输入捕获超声波测距.md](docs/09-输入捕获超声波测距.md) |
| 09 | 主从模式 + PWM 测量 | [docs/10-主从模式PWM测量.md](docs/10-主从模式PWM测量.md) |
| 10 | ADC 光敏传感器 + 回调函数 | [docs/11-ADC光敏传感器.md](docs/11-ADC光敏传感器.md) |
| 11 | ADC 注入序列 + 扫描模式 | [docs/14-ADC注入序列+扫描模式.md](docs/14-ADC注入序列+扫描模式.md) |
| 12 | FreeRTOS 入门 | [docs/15-FreeRTOS入门.md](docs/15-FreeRTOS入门.md) |
| 13 | FreeRTOS 队列实战 | [docs/16-FreeRTOS队列实战.md](docs/16-FreeRTOS队列实战.md) |
| 14 | 二进制信号量 | [docs/17-二进制信号量.md](docs/17-二进制信号量.md) |
| 15 | 综合项目 RTOS 重构 | [docs/18-综合项目RTOS重构.md](docs/18-综合项目RTOS重构.md) |
| 16 | 按键消抖模块化封装 | [docs/19-按键消抖模块化.md](docs/19-按键消抖模块化.md) |
| 17 | 模块化头文件 & DMA 调试 | [docs/22-模块化头文件与DMA调试.md](docs/22-模块化头文件与DMA调试.md) |

---

## 📚 心得分享

- [外设中英文对照表](心得分享/外设中英文对照表.md) — 13 大类，180+ 术语

---

## 🛠 外设覆盖

- [x] GPIO（推挽/开漏/上拉/下拉）
- [x] USART（重映射、中断接收、DMA 收发、CLI 命令）
- [x] I2C（主机收发、ACK/NAK、时钟拉伸）
- [x] SPI（全双工、W25Q64 Flash、阈值持久化）
- [x] NVIC + EXTI（优先级分组、外部中断）
- [x] TIM（时基/PWM/输入捕获/主从模式）
- [x] ADC（常规/注入/扫描/定时器触发）
- [x] OLED（I2C 驱动、多页面切换）
- [x] FreeRTOS（任务、队列、信号量、互斥锁）
- [x] DWT（微秒级延时）
- [x] CLI（命令行交互控制）


