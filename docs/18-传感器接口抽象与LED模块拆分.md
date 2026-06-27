# 6.27 心得 —— LED 模块拆分 & 传感器接口抽象

> 持续更新 | 最后更新：2026-06-27

---

## 一、LED 模块拆分：单一职责

之前 `led_task.c/h` 一个文件管了三件事（PA3 可控灯、PA11 心跳灯、PC13 指示灯），这次按职责拆成三个独立模块：

| 模块 | 文件 | 职责 |
|------|------|------|
| 可控 LED | `Controllable_led_task.c/h` | PA3 红光 LED，响应 CLI/队列命令（ON/OFF/Flicker） |
| 心跳 LED | `Heartbeat_led.c/h` | PA11 蓝光 LED，500ms 周期闪烁，纯装饰 |
| 状态指示灯 | `Status_indication_led.c/h` | PC13 板载 LED，根据传感器阈值亮/灭 |

**好处**：每个模块只做一件事，改哪个不会影响另外两个，模块开关也更精准。

---

## 二、传感器接口抽象

新增 `sensor_interface.h`，用函数指针把传感器抽象成统一接口：

```c
struct Sensor {
    const char *name;                       // 传感器名
    const char *display_name;               // 别名
    int   (*read)(Sensor *me);              // 触发一次测量，0=成功
    float (*get_value)(Sensor *me);         // 拿最新值
    const char *(*get_uint)(Sensor *me);    // 返回单位字符串
};
```

光敏传感器和超声波模块各自实现这个结构体，CLI 和 OLED 调用时不需要知道是哪个传感器——多态的思想在嵌入式里的落地。

---

## 三、通知机制替换轮询

之前 `Status_indication_led` 用轮询检查传感器值是否超阈值，改为通知机制：

```
传感器数据更新 → Sensor_Notify_QueueHandle → Status_indication_led 被唤醒
```

新增：
- `OLED_Display_QueueHandle`：OLED 页面刷新队列，解耦显示请求
- `Sensor_Notify_QueueHandle`：传感器更新通知队列，替代轮询
- `SensorEvent` 枚举：`SENSOR_PS_UPDATED` / `SENSOR_UR_UPDATED`

从轮询到通知，CPU 不再空转检查传感器状态。

---

## 四、错误处理模块化

新增 `app_common_err.c/h`，定义了传感器统一错误码：

```c
#define UR_ERR_WAIT_ECHO_TIMEOUT  -1.0f   // 超声波超时
#define UR_ERR_OUTRANGE           -2.0f   // 超出范围
#define PS_ERR_Wait_ADC_JEOC_TIMEOUT -1.0f // 光敏超时
```

可选启用 `ERR_AUTOMATIC_PRINTING_MODULE_ENABLED`，自动打印错误信息。

---

## 五、今日架构演进对比

| 维度 | 之前 (6.24) | 现在 (6.27) |
|------|-------------|-------------|
| LED 模块 | 1 个文件管 3 种灯 | 3 个独立模块 |
| 传感器 | 各自为政 | 统一 `Sensor` 接口 |
| 状态通知 | 轮询 | 队列通知 |
| 错误处理 | 分散在各模块 | 统一错误码 + 自动打印 |
