# 6.15 FreeRTOS 入门 — 首个多任务 + 架构全景

> **芯片**：STM32F103C8T6 | **环境**：CubeMX + VSCode + Keil  
> **主题**：FreeRTOS 工程搭建、三任务调度、内核架构、C 语言内存/作用域强化

---

## 一、核心代码

### 1. C 语言强化：静态局部变量 + 动态内存

```c
// 静态局部变量：函数退出不销毁，值保留
static int static_globalvar = 100;  // 静态全局变量 — 仅本文件可见

int getnextsquare()
{
    static int num = 1;     // 静态局部：生命周期 = 整个程序
    int ret = num * num;
    num++;
    return ret;
}

// 主函数：动态分配 n 个平方数
int main(int argc, char *argv[])
{
    int n;
    scanf_s("%d", &n);
    int *pbuff = (int *)malloc(sizeof(int) * n);  // 堆变量
    for (int i = 0; i < n; i++)
        pbuff[i] = getnextsquare();
    // ... 打印 ...
    free(pbuff);  // 好习惯：释放堆内存
    return 0;
}
```

> 变量四象限：全局 → 静态全局 → 静态局部 → 堆(malloc) → 局部(栈)

### 2. FreeRTOS 首个多任务

```c
#include "FreeRTOS.h"
#include "task.h"

// 任务1：LED1 100ms 闪烁
void vLED1Task(void *argument)
{
    while (1) {
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
        vTaskDelay(pdMS_TO_TICKS(100));
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// 任务2：LED2 30ms/150ms 非对称闪烁
void vLED2Task(void *argument)
{
    while (1) {
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
        vTaskDelay(pdMS_TO_TICKS(30));
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
        vTaskDelay(pdMS_TO_TICKS(150));
    }
}

// 任务3：串口每 1s 发送 "你好世界"
void usart_send_helloworld(void *task)
{
    while (1) {
        HAL_UART_Transmit(&huart1, (uint8_t*)"你好世界\n", strlen(...), HAL_MAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// 启动
xTaskCreate(vLED1Task, "LED1", 128, NULL, 1, NULL);
xTaskCreate(vLED2Task, "LED2", 128, NULL, 1, NULL);
xTaskCreate(usart_send_helloworld, "UART", 128, NULL, 1, NULL);
vTaskStartScheduler();  // 调度器启动，永不返回
```

---

## 二、核心知识点

### 1. FreeRTOS 三大硬件基石

| 异常 | 作用 | 优先级 | 口诀 |
|------|------|--------|------|
| **SysTick** | 提供 tick 时钟，驱动时间片 | 最低 | 报时 |
| **SVC** | 启动第一个任务 | **最高** | 起跑 |
| **PendSV** | 执行上下文切换 | 最低 | 换场不打扰 |

> PendSV 优先级最低，保证所有 ISR 执行完毕再切换——这是 RTOS 设计的精髓

### 2. 调度器三种模式

| 模式 | 行为 |
|------|------|
| 带时间片抢占 | 同优先级轮流执行（`configUSE_TIME_SLICING=1`） |
| 不带时间片抢占 | 高优先级立即抢占，同级需主动让出 CPU |
| 协作式 | 任务必须主动释放，否则独占 |

### 3. xTaskCreate 六参数

```
xTaskCreate(任务函数, "名称", 栈深度(字), 参数, 优先级, 句柄)
```

| 参数 | 说明 | 常见坑 |
|------|------|--------|
| 栈深度 | 单位 word(4字节) | 设太小 → 栈溢出，建议 ×1.5~2 倍估算 |
| 优先级 | 数值越大越高 | 0 是空闲任务优先级 |

### 4. 五种堆方案（heap_1~5）

| 方案 | 特点 | 适用 |
|------|------|------|
| heap_1 | 只分配不释放 | 静态任务 |
| heap_2 | 可释放，不合并碎片 | 已过时 |
| heap_3 | 封装 malloc/free | 需保证线程安全 |
| **heap_4** | **合并空闲块** | **最常用** |
| heap_5 | heap_4 + 跨多块内存 | 有外部 SRAM 时 |

### 5. FreeRTOSConfig.h 关键配置

```c
#define configCPU_CLOCK_HZ                    72000000   // 主频
#define configTICK_TYPE_WIDTH_IN_BITS         TICK_TYPE_WIDTH_32_BITS
#define configKERNEL_INTERRUPT_PRIORITY       (15 << 4)  // ST只用高4位
#define configMAX_SYSCALL_INTERRUPT_PRIORITY  (5 << 4)   // 高于此不受API干扰
#define configUSE_TIME_SLICING                1
#define configCHECK_FOR_STACK_OVERFLOW        0           // 调试阶段暂时关闭
```

### 6. 任务间通信全家福

```
队列 ──────────── 数据传输（快递传送带）
二进制信号量 ──── 同步（红绿灯）
计数信号量 ────── 资源管理（停车位闸机）
互斥量 ────────── 优先级反转保护
事件组 ────────── 多条件与/或触发
任务通知 ──────── 轻量级单值邮箱
```

### 7. 变量四象限（面试高频）

| 类型 | 作用域 | 生命周期 | 存储区 |
|------|--------|----------|--------|
| 全局变量 | 所有文件 | 整个程序 | 数据段 |
| 静态全局 | 当前文件 | 整个程序 | 数据段 |
| **静态局部** | 当前函数 | **整个程序** | 数据段 |
| 堆变量 | 有指针处 | free 为止 | 堆 |
| 局部变量 | 当前函数 | 函数退出 | 栈 |

> 静态局部变量不随函数压栈出栈——编译时分配在数据段，固定地址

---

## 三、心得

- 裸机状态机 → FreeRTOS 任务，不是推倒重来，是把 `switch-case` 升级为系统帮你调度
- SysTick/SVC/PendSV 三个异常是 RTOS 的全部硬件依赖——理解它们就理解了"操作系统"的本质
- 栈深度 128 是经验起点，正式项目用 `uxTaskGetStackHighWaterMark()` 实测后再调优