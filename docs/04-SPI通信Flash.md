# 6.4 SPI 通信 + W25Q64 Flash

> **芯片**：STM32F103C8T6 | **环境**：VSCode + Keil + Keil Assistant  
> **主题**：SPI 全双工、W25Q64 Flash 读写、JTAG 释放、借时钟原理、I2C vs SPI 对比

---

## 一、核心代码

### 1. SPI 初始化（释放 JTAG + 重映射）

```c
void init_SPI_GPIO(void)
{
    // 释放 PA15（JTAG JTDI → 普通 GPIO）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SPI1, ENABLE);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitTypeDef g = {0};

    // PB3(SCK) + PB5(MOSI) → AF_PP
    g.GPIO_Pin   = GPIO_Pin_3 | GPIO_Pin_5;
    g.GPIO_Speed = GPIO_Speed_2MHz;  // 面包板低速防振铃
    g.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &g);

    // PB4(MISO) → IPU
    g.GPIO_Pin  = GPIO_Pin_4;
    g.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &g);

    // PA15(CS) → Out_PP，默认拉高（未选中）
    g.GPIO_Pin  = GPIO_Pin_15;
    g.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &g);
    GPIO_WriteBit(GPIOA, GPIO_Pin_15, Bit_SET);
}

void init_SPI(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    SPI_InitTypeDef s = {0};
    s.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
    s.SPI_Mode              = SPI_Mode_Master;
    s.SPI_CPHA              = SPI_CPHA_1Edge;   // 模式0
    s.SPI_CPOL              = SPI_CPOL_Low;
    s.SPI_DataSize          = SPI_DataSize_8b;
    s.SPI_FirstBit          = SPI_FirstBit_MSB;
    s.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;  // 72M/64=1.125M
    s.SPI_NSS               = SPI_NSS_Soft;
    SPI_Init(SPI1, &s);
    SPI_NSSInternalSoftwareConfig(SPI1, SPI_NSSInternalSoft_Set);
}
```

### 2. SPI 全双工收发（动态开关 + BSY 检查）

```c
int SPI_Send_and_Receive(SPI_TypeDef *SPIx, const uint8_t *TXData,
                          uint8_t *RXData, uint16_t Size, int32_t TimeOut)
{
    if (TXData == NULL || RXData == NULL || Size <= 0) return ERR;

    SPI_Cmd(SPIx, ENABLE);  // 用时开

    for (uint16_t i = 0; i < Size; i++) {
        // 等 TXE → 发 → 等 RXNE → 收
        while (SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_TXE) == RESET);
        SPI_I2S_SendData(SPIx, TXData[i]);
        while (SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_RXNE) == RESET);
        RXData[i] = SPI_I2S_ReceiveData(SPIx);
    }

    // 必须等 BSY=0，确保移位寄存器清空
    while (SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_BSY) == SET);
    SPI_Cmd(SPIx, DISABLE);  // 用完关
    return SPI_SUCCESS;
}
```

### 3. W25Q64 写一个字节

```c
void SPI_W25Q64_Save_Byte(uint8_t Byte)
{
    uint8_t buf[10];

    // ① 写使能
    buf[0] = 0x06;
    CS_LOW();  SPI_Send_and_Receive(SPI1, buf, buf, 1, 50);  CS_HIGH();

    // ② 扇区擦除（0x20 + 24位地址）
    buf[0]=0x20; buf[1]=0x00; buf[2]=0x00; buf[3]=0x00;
    CS_LOW();  SPI_Send_and_Receive(SPI1, buf, buf, 4, 50);  CS_HIGH();

    // ③ 等空闲（读状态寄存器 bit0）
    do {
        buf[0]=0x05; CS_LOW(); SPI_Send_and_Receive(SPI1, buf, buf, 1, 50);
        buf[0]=0xFF; SPI_Send_and_Receive(SPI1, buf, buf, 1, 50); CS_HIGH();
    } while ((buf[0] & 0x01) != 0);

    // ④ 再次写使能
    buf[0]=0x06; CS_LOW(); SPI_Send_and_Receive(SPI1, buf, buf, 1, 50); CS_HIGH();

    // ⑤ 页编程（0x02 + 地址 + 数据）
    buf[0]=0x02; buf[1]=0x00; buf[2]=0x00; buf[3]=0x00; buf[4]=Byte;
    CS_LOW();  SPI_Send_and_Receive(SPI1, buf, buf, 5, 50);  CS_HIGH();

    // ⑥ 等空闲
    do {
        buf[0]=0x05; CS_LOW(); SPI_Send_and_Receive(SPI1, buf, buf, 1, 50);
        buf[0]=0xFF; SPI_Send_and_Receive(SPI1, buf, buf, 1, 50); CS_HIGH();
    } while ((buf[0] & 0x01) != 0);
}
```

### 4. W25Q64 读一个字节（借时钟）

```c
uint8_t SPI_W25Q64_Read_Byte(void)
{
    uint8_t buf[10];
    CS_LOW();
    buf[0]=0x03; buf[1]=0x00; buf[2]=0x00; buf[3]=0x00;
    SPI_Send_and_Receive(SPI1, buf, buf, 4, 50);  // 发命令+地址
    buf[0]=0xFF;  // dummy 字节 — 借时钟给从机输出数据
    SPI_Send_and_Receive(SPI1, buf, buf, 1, 50);
    CS_HIGH();
    return buf[0];
}
```

### 5. 验证：写入 0x12 → 读出比对 → LED 反馈

```c
void Process_SPI(void)
{
    init_SPI_GPIO(); init_SPI(); init_PC13_GPIO(); init_Button_GPIO();

    SPI_W25Q64_Save_Byte(0x12);
    uint8_t a = SPI_W25Q64_Read_Byte();

    if (a == 0x12) {
        // 正确 → LED 常亮，直到按键按下
        while (1) {
            GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);
            if (KEY_PRESSED()) break;  // 消抖后退出
        }
    }
    GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);  // 灭灯
}
```

---

## 二、核心知识点

### 1. SPI 四线引脚

| 引脚 | 方向 | 功能 | 模式 |
|------|------|------|------|
| **SCK** | 主→从 | 时钟 | AF_PP |
| **MOSI** | 主→从 | 数据输出 | AF_PP |
| **MISO** | 从→主 | 数据输入 | IPU |
| **NSS/CS** | 主→从 | 片选（软件控制） | Out_PP |

### 2. SPI 模式（CPOL/CPHA）

| 模式 | CPOL | CPHA | W25Q64 |
|------|------|------|--------|
| 0 | 0（空闲低） | 0（第1边沿采样） | ✅ |
| 1 | 0 | 1（第2边沿） | |
| 2 | 1（空闲高） | 0 | |
| 3 | 1 | 1 | |

### 3. 借时钟原理（读 Flash 为什么要发 0xFF）

```
SPI 时钟完全由主机产生。发送 0x03+地址后，从机已准备好数据，
但没有主机时钟就无法输出。发 0xFF（或其他任意值）= 产生 8 个时钟，
从机趁机把数据推到 MISO 上。
```

### 4. W25Q64 写流程

```
写使能(0x06) → 扇区擦除(0x20+24位地址) → 等BUSY=0 → 写使能 → 页编程(0x02+地址+数据) → 等BUSY=0
```

> 扇区擦除是必须的：Flash 只能把 1 写成 0，要写新数据必须先擦除（全置 1）

### 5. 状态寄存器读取技巧

```c
buf[0] = 0x05;  SPI_Send_and_Receive(SPI1, buf, buf, 1, 50);  // 发命令
buf[0] = 0xFF;  SPI_Send_and_Receive(SPI1, buf, buf, 1, 50);  // 借时钟取状态
// 此时 buf[0] 才是真正的状态值
```

- BUSY 位在 bit0：`(buf[0] & 0x01) == 0` 表示空闲
- 注意 `==` 优先级高于 `&`，必须加括号

### 6. SPI 动态开关范式

```c
SPI_Cmd(ENABLE);   // 收发前开
/* ... 收发操作 ... */
while (BSY == SET); // 等移位寄存器清空
SPI_Cmd(DISABLE);  // 收发后关
```

- 避免空闲时总线状态不确定
- 最后一字节可能还在移位寄存器中，务必等 BSY

### 7. I2C vs SPI 对比

| 特性 | I2C | SPI |
|------|-----|-----|
| 连线 | 2 线（SCL/SDA） | 4 线（SCK/MOSI/MISO/CS） |
| 寻址 | 起始条件后发地址 | 无地址，靠 CS 引脚选从机 |
| 双工 | 半双工 | **全双工**（同时收发） |
| 流控 | 时钟拉伸（从机拉低 SCL） | 无从机流控 |
| 应答 | 每字节后 ACK/NAK | 无逐字节应答 |
| 速度 | 100k/400k/3.4M | 最高数十 MHz |
| 帧定界 | START/STOP 条件 | CS 高/低电平 |

### 8. PA15 释放 JTAG

```c
RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
// PA15 此时变为普通 GPIO，可作 CS 使用
```

---

## 三、心得

- SPI 比 I2C 简单但更底层：全双工意味着**发送和接收同时发生**，理解这一点才能懂"借时钟"
- Flash 写操作是嵌入式最经典的**命令序列**：每一步有严格的先后依赖，出错就是数据丢失
- 面包板环境 2MHz + 64 分频 = 1.125MHz 是经验值，工程中"能用低速就不用高速"
- `SPI_Cmd` 动态开关值得养成习惯，省电且安全