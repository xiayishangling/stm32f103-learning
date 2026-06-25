# 6.4 SPI 閫氫俊 + W25Q64 Flash

> **鑺墖**锛歋TM32F103C8T6 | **鐜**锛歏SCode + Keil + Keil Assistant  
> **涓婚**锛歋PI 鍏ㄥ弻宸ャ€乄25Q64 Flash 璇诲啓銆丣TAG 閲婃斁銆佸€熸椂閽熷師鐞嗐€両2C vs SPI 瀵规瘮

---

## 涓€銆佹牳蹇冧唬鐮?
### 1. SPI 鍒濆鍖栵紙閲婃斁 JTAG + 閲嶆槧灏勶級

```c
void init_SPI_GPIO(void)
{
    // 閲婃斁 PA15锛圝TAG JTDI 鈫?鏅€?GPIO锛?    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SPI1, ENABLE);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitTypeDef g = {0};

    // PB3(SCK) + PB5(MOSI) 鈫?AF_PP
    g.GPIO_Pin   = GPIO_Pin_3 | GPIO_Pin_5;
    g.GPIO_Speed = GPIO_Speed_2MHz;  // 闈㈠寘鏉夸綆閫熼槻鎸搩
    g.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &g);

    // PB4(MISO) 鈫?IPU
    g.GPIO_Pin  = GPIO_Pin_4;
    g.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &g);

    // PA15(CS) 鈫?Out_PP锛岄粯璁ゆ媺楂橈紙鏈€変腑锛?    g.GPIO_Pin  = GPIO_Pin_15;
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
    s.SPI_CPHA              = SPI_CPHA_1Edge;   // 妯″紡0
    s.SPI_CPOL              = SPI_CPOL_Low;
    s.SPI_DataSize          = SPI_DataSize_8b;
    s.SPI_FirstBit          = SPI_FirstBit_MSB;
    s.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;  // 72M/64=1.125M
    s.SPI_NSS               = SPI_NSS_Soft;
    SPI_Init(SPI1, &s);
    SPI_NSSInternalSoftwareConfig(SPI1, SPI_NSSInternalSoft_Set);
}
```

### 2. SPI 鍏ㄥ弻宸ユ敹鍙戯紙鍔ㄦ€佸紑鍏?+ BSY 妫€鏌ワ級

```c
int SPI_Send_and_Receive(SPI_TypeDef *SPIx, const uint8_t *TXData,
                          uint8_t *RXData, uint16_t Size, int32_t TimeOut)
{
    if (TXData == NULL || RXData == NULL || Size <= 0) return ERR;

    SPI_Cmd(SPIx, ENABLE);  // 鐢ㄦ椂寮€

    for (uint16_t i = 0; i < Size; i++) {
        // 绛?TXE 鈫?鍙?鈫?绛?RXNE 鈫?鏀?        while (SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_TXE) == RESET);
        SPI_I2S_SendData(SPIx, TXData[i]);
        while (SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_RXNE) == RESET);
        RXData[i] = SPI_I2S_ReceiveData(SPIx);
    }

    // 蹇呴』绛?BSY=0锛岀‘淇濈Щ浣嶅瘎瀛樺櫒娓呯┖
    while (SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_BSY) == SET);
    SPI_Cmd(SPIx, DISABLE);  // 鐢ㄥ畬鍏?    return SPI_SUCCESS;
}
```

### 3. W25Q64 鍐欎竴涓瓧鑺?
```c
void SPI_W25Q64_Save_Byte(uint8_t Byte)
{
    uint8_t buf[10];

    // 鈶?鍐欎娇鑳?    buf[0] = 0x06;
    CS_LOW();  SPI_Send_and_Receive(SPI1, buf, buf, 1, 50);  CS_HIGH();

    // 鈶?鎵囧尯鎿﹂櫎锛?x20 + 24浣嶅湴鍧€锛?    buf[0]=0x20; buf[1]=0x00; buf[2]=0x00; buf[3]=0x00;
    CS_LOW();  SPI_Send_and_Receive(SPI1, buf, buf, 4, 50);  CS_HIGH();

    // 鈶?绛夌┖闂诧紙璇荤姸鎬佸瘎瀛樺櫒 bit0锛?    do {
        buf[0]=0x05; CS_LOW(); SPI_Send_and_Receive(SPI1, buf, buf, 1, 50);
        buf[0]=0xFF; SPI_Send_and_Receive(SPI1, buf, buf, 1, 50); CS_HIGH();
    } while ((buf[0] & 0x01) != 0);

    // 鈶?鍐嶆鍐欎娇鑳?    buf[0]=0x06; CS_LOW(); SPI_Send_and_Receive(SPI1, buf, buf, 1, 50); CS_HIGH();

    // 鈶?椤电紪绋嬶紙0x02 + 鍦板潃 + 鏁版嵁锛?    buf[0]=0x02; buf[1]=0x00; buf[2]=0x00; buf[3]=0x00; buf[4]=Byte;
    CS_LOW();  SPI_Send_and_Receive(SPI1, buf, buf, 5, 50);  CS_HIGH();

    // 鈶?绛夌┖闂?    do {
        buf[0]=0x05; CS_LOW(); SPI_Send_and_Receive(SPI1, buf, buf, 1, 50);
        buf[0]=0xFF; SPI_Send_and_Receive(SPI1, buf, buf, 1, 50); CS_HIGH();
    } while ((buf[0] & 0x01) != 0);
}
```

### 4. W25Q64 璇讳竴涓瓧鑺傦紙鍊熸椂閽燂級

```c
uint8_t SPI_W25Q64_Read_Byte(void)
{
    uint8_t buf[10];
    CS_LOW();
    buf[0]=0x03; buf[1]=0x00; buf[2]=0x00; buf[3]=0x00;
    SPI_Send_and_Receive(SPI1, buf, buf, 4, 50);  // 鍙戝懡浠?鍦板潃
    buf[0]=0xFF;  // dummy 瀛楄妭 鈥?鍊熸椂閽熺粰浠庢満杈撳嚭鏁版嵁
    SPI_Send_and_Receive(SPI1, buf, buf, 1, 50);
    CS_HIGH();
    return buf[0];
}
```

### 5. 楠岃瘉锛氬啓鍏?0x12 鈫?璇诲嚭姣斿 鈫?LED 鍙嶉

```c
void Process_SPI(void)
{
    init_SPI_GPIO(); init_SPI(); init_PC13_GPIO(); init_Button_GPIO();

    SPI_W25Q64_Save_Byte(0x12);
    uint8_t a = SPI_W25Q64_Read_Byte();

    if (a == 0x12) {
        // 姝ｇ‘ 鈫?LED 甯镐寒锛岀洿鍒版寜閿寜涓?        while (1) {
            GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);
            if (KEY_PRESSED()) break;  // 娑堟姈鍚庨€€鍑?        }
    }
    GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);  // 鐏伅
}
```

---

## 浜屻€佹牳蹇冪煡璇嗙偣

### 1. SPI 鍥涚嚎寮曡剼

| 寮曡剼 | 鏂瑰悜 | 鍔熻兘 | 妯″紡 |
|------|------|------|------|
| **SCK** | 涓烩啋浠?| 鏃堕挓 | AF_PP |
| **MOSI** | 涓烩啋浠?| 鏁版嵁杈撳嚭 | AF_PP |
| **MISO** | 浠庘啋涓?| 鏁版嵁杈撳叆 | IPU |
| **NSS/CS** | 涓烩啋浠?| 鐗囬€夛紙杞欢鎺у埗锛?| Out_PP |

### 2. SPI 妯″紡锛圕POL/CPHA锛?
| 妯″紡 | CPOL | CPHA | W25Q64 |
|------|------|------|--------|
| 0 | 0锛堢┖闂蹭綆锛?| 0锛堢1杈规部閲囨牱锛?| 鉁?|
| 1 | 0 | 1锛堢2杈规部锛?| |
| 2 | 1锛堢┖闂查珮锛?| 0 | |
| 3 | 1 | 1 | |

### 3. 鍊熸椂閽熷師鐞嗭紙璇?Flash 涓轰粈涔堣鍙?0xFF锛?
```
SPI 鏃堕挓瀹屽叏鐢变富鏈轰骇鐢熴€傚彂閫?0x03+鍦板潃鍚庯紝浠庢満宸插噯澶囧ソ鏁版嵁锛?浣嗘病鏈変富鏈烘椂閽熷氨鏃犳硶杈撳嚭銆傚彂 0xFF锛堟垨鍏朵粬浠绘剰鍊硷級= 浜х敓 8 涓椂閽燂紝
浠庢満瓒佹満鎶婃暟鎹帹鍒?MISO 涓娿€?```

### 4. W25Q64 鍐欐祦绋?
```
鍐欎娇鑳?0x06) 鈫?鎵囧尯鎿﹂櫎(0x20+24浣嶅湴鍧€) 鈫?绛塀USY=0 鈫?鍐欎娇鑳?鈫?椤电紪绋?0x02+鍦板潃+鏁版嵁) 鈫?绛塀USY=0
```

> 鎵囧尯鎿﹂櫎鏄繀椤荤殑锛欶lash 鍙兘鎶?1 鍐欐垚 0锛岃鍐欐柊鏁版嵁蹇呴』鍏堟摝闄わ紙鍏ㄧ疆 1锛?
### 5. 鐘舵€佸瘎瀛樺櫒璇诲彇鎶€宸?
```c
buf[0] = 0x05;  SPI_Send_and_Receive(SPI1, buf, buf, 1, 50);  // 鍙戝懡浠?buf[0] = 0xFF;  SPI_Send_and_Receive(SPI1, buf, buf, 1, 50);  // 鍊熸椂閽熷彇鐘舵€?// 姝ゆ椂 buf[0] 鎵嶆槸鐪熸鐨勭姸鎬佸€?```

- BUSY 浣嶅湪 bit0锛歚(buf[0] & 0x01) == 0` 琛ㄧず绌洪棽
- 娉ㄦ剰 `==` 浼樺厛绾ч珮浜?`&`锛屽繀椤诲姞鎷彿

### 6. SPI 鍔ㄦ€佸紑鍏宠寖寮?
```c
SPI_Cmd(ENABLE);   // 鏀跺彂鍓嶅紑
/* ... 鏀跺彂鎿嶄綔 ... */
while (BSY == SET); // 绛夌Щ浣嶅瘎瀛樺櫒娓呯┖
SPI_Cmd(DISABLE);  // 鏀跺彂鍚庡叧
```

- 閬垮厤绌洪棽鏃舵€荤嚎鐘舵€佷笉纭畾
- 鏈€鍚庝竴瀛楄妭鍙兘杩樺湪绉讳綅瀵勫瓨鍣ㄤ腑锛屽姟蹇呯瓑 BSY

### 7. I2C vs SPI 瀵规瘮

| 鐗规€?| I2C | SPI |
|------|-----|-----|
| 杩炵嚎 | 2 绾匡紙SCL/SDA锛?| 4 绾匡紙SCK/MOSI/MISO/CS锛?|
| 瀵诲潃 | 璧峰鏉′欢鍚庡彂鍦板潃 | 鏃犲湴鍧€锛岄潬 CS 寮曡剼閫変粠鏈?|
| 鍙屽伐 | 鍗婂弻宸?| **鍏ㄥ弻宸?*锛堝悓鏃舵敹鍙戯級 |
| 娴佹帶 | 鏃堕挓鎷変几锛堜粠鏈烘媺浣?SCL锛?| 鏃犱粠鏈烘祦鎺?|
| 搴旂瓟 | 姣忓瓧鑺傚悗 ACK/NAK | 鏃犻€愬瓧鑺傚簲绛?|
| 閫熷害 | 100k/400k/3.4M | 鏈€楂樻暟鍗?MHz |
| 甯у畾鐣?| START/STOP 鏉′欢 | CS 楂?浣庣數骞?|

### 8. PA15 閲婃斁 JTAG

```c
RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
// PA15 姝ゆ椂鍙樹负鏅€?GPIO锛屽彲浣?CS 浣跨敤
```

---

## 涓夈€佸績寰?
- SPI 姣?I2C 绠€鍗曚絾鏇村簳灞傦細鍏ㄥ弻宸ユ剰鍛崇潃**鍙戦€佸拰鎺ユ敹鍚屾椂鍙戠敓**锛岀悊瑙ｈ繖涓€鐐规墠鑳芥噦"鍊熸椂閽?
- Flash 鍐欐搷浣滄槸宓屽叆寮忔渶缁忓吀鐨?*鍛戒护搴忓垪**锛氭瘡涓€姝ユ湁涓ユ牸鐨勫厛鍚庝緷璧栵紝鍑洪敊灏辨槸鏁版嵁涓㈠け
- 闈㈠寘鏉跨幆澧?2MHz + 64 鍒嗛 = 1.125MHz 鏄粡楠屽€硷紝宸ョ▼涓?鑳界敤浣庨€熷氨涓嶇敤楂橀€?
- `SPI_Cmd` 鍔ㄦ€佸紑鍏冲€煎緱鍏绘垚涔犳儻锛岀渷鐢典笖瀹夊叏
