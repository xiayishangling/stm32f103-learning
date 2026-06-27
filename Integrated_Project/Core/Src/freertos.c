/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "app_globals.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for KEY_Task */
osThreadId_t KEY_TaskHandle;
const osThreadAttr_t KEY_Task_attributes = {
  .name = "KEY_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for LED_Task */
osThreadId_t LED_TaskHandle;
const osThreadAttr_t LED_Task_attributes = {
  .name = "LED_Task",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for USART_Task */
osThreadId_t USART_TaskHandle;
const osThreadAttr_t USART_Task_attributes = {
  .name = "USART_Task",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh1,
};
/* Definitions for OLED_Task */
osThreadId_t OLED_TaskHandle;
const osThreadAttr_t OLED_Task_attributes = {
  .name = "OLED_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal1,
};
/* Definitions for W25Q64_Task */
osThreadId_t W25Q64_TaskHandle;
const osThreadAttr_t W25Q64_Task_attributes = {
  .name = "W25Q64_Task",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal2,
};
/* Definitions for PA11Flick_Task */
osThreadId_t PA11Flick_TaskHandle;
const osThreadAttr_t PA11Flick_Task_attributes = {
  .name = "PA11Flick_Task",
  .stack_size = 64 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for Test_CPU_usage */
osThreadId_t Test_CPU_usageHandle;
const osThreadAttr_t Test_CPU_usage_attributes = {
  .name = "Test_CPU_usage",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for PS_Task */
osThreadId_t PS_TaskHandle;
const osThreadAttr_t PS_Task_attributes = {
  .name = "PS_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for UR_Task */
osThreadId_t UR_TaskHandle;
const osThreadAttr_t UR_Task_attributes = {
  .name = "UR_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for CLI_Task */
osThreadId_t CLI_TaskHandle;
const osThreadAttr_t CLI_Task_attributes = {
  .name = "CLI_Task",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for PC13_Task */
osThreadId_t PC13_TaskHandle;
const osThreadAttr_t PC13_Task_attributes = {
  .name = "PC13_Task",
  .stack_size = 64 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for ERR_Task */
osThreadId_t ERR_TaskHandle;
const osThreadAttr_t ERR_Task_attributes = {
  .name = "ERR_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for KEY_Queue */
osMessageQueueId_t KEY_QueueHandle;
const osMessageQueueAttr_t KEY_Queue_attributes = {
  .name = "KEY_Queue"
};
/* Definitions for USART_Queue */
osMessageQueueId_t USART_QueueHandle;
const osMessageQueueAttr_t USART_Queue_attributes = {
  .name = "USART_Queue"
};
/* Definitions for OLED_Display_Queue */
osMessageQueueId_t OLED_Display_QueueHandle;
const osMessageQueueAttr_t OLED_Display_Queue_attributes = {
  .name = "OLED_Display_Queue"
};
/* Definitions for Sensor_Notify_Queue */
osMessageQueueId_t Sensor_Notify_QueueHandle;
const osMessageQueueAttr_t Sensor_Notify_Queue_attributes = {
  .name = "Sensor_Notify_Queue"
};
/* Definitions for General_Mutex */
osMutexId_t General_MutexHandle;
const osMutexAttr_t General_Mutex_attributes = {
  .name = "General_Mutex"
};
/* Definitions for KEY_BinarySem */
osSemaphoreId_t KEY_BinarySemHandle;
const osSemaphoreAttr_t KEY_BinarySem_attributes = {
  .name = "KEY_BinarySem"
};
/* Definitions for PS_BinarySem */
osSemaphoreId_t PS_BinarySemHandle;
const osSemaphoreAttr_t PS_BinarySem_attributes = {
  .name = "PS_BinarySem"
};
/* Definitions for UR_BinarySem */
osSemaphoreId_t UR_BinarySemHandle;
const osSemaphoreAttr_t UR_BinarySem_attributes = {
  .name = "UR_BinarySem"
};
/* Definitions for USART_CountingSem */
osSemaphoreId_t USART_CountingSemHandle;
const osSemaphoreAttr_t USART_CountingSem_attributes = {
  .name = "USART_CountingSem"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void vIP_KEYTask(void *argument);
void vIP_LEDTask(void *argument);
void vIP_USARTTask(void *argument);
void vIP_OLEDTask(void *argument);
void vIP_W25Q64Task(void *argument);
void vIP_PA11Flick(void *argument);
void T_CPU_U_Task(void *argument);
void vIP_PS_IJ_Task(void *argument);
void vIP_UR_Task(void *argument);
void vIP_CLI_Task(void *argument);
void vIP_PC13_StartTask(void *argument);
void ERR_Printf_Task(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of General_Mutex */
  General_MutexHandle = osMutexNew(&General_Mutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of KEY_BinarySem */
  KEY_BinarySemHandle = osSemaphoreNew(1, 0, &KEY_BinarySem_attributes);

  /* creation of PS_BinarySem */
  PS_BinarySemHandle = osSemaphoreNew(1, 1, &PS_BinarySem_attributes);

  /* creation of UR_BinarySem */
  UR_BinarySemHandle = osSemaphoreNew(1, 0, &UR_BinarySem_attributes);

  /* creation of USART_CountingSem */
  USART_CountingSemHandle = osSemaphoreNew(10, 0, &USART_CountingSem_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of KEY_Queue */
  KEY_QueueHandle = osMessageQueueNew (16, sizeof(KEYMessage), &KEY_Queue_attributes);

  /* creation of USART_Queue */
  USART_QueueHandle = osMessageQueueNew (16, sizeof(USARTMessage), &USART_Queue_attributes);

  /* creation of OLED_Display_Queue */
  OLED_Display_QueueHandle = osMessageQueueNew (16, sizeof(OLEDDisplayRequest), &OLED_Display_Queue_attributes);

  /* creation of Sensor_Notify_Queue */
  Sensor_Notify_QueueHandle = osMessageQueueNew (8, sizeof(SensorNotifyMsg), &Sensor_Notify_Queue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of KEY_Task */
  KEY_TaskHandle = osThreadNew(vIP_KEYTask, NULL, &KEY_Task_attributes);

  /* creation of LED_Task */
  LED_TaskHandle = osThreadNew(vIP_LEDTask, NULL, &LED_Task_attributes);

  /* creation of USART_Task */
  USART_TaskHandle = osThreadNew(vIP_USARTTask, NULL, &USART_Task_attributes);

  /* creation of OLED_Task */
  OLED_TaskHandle = osThreadNew(vIP_OLEDTask, NULL, &OLED_Task_attributes);

  /* creation of W25Q64_Task */
  W25Q64_TaskHandle = osThreadNew(vIP_W25Q64Task, NULL, &W25Q64_Task_attributes);

  /* creation of PA11Flick_Task */
  PA11Flick_TaskHandle = osThreadNew(vIP_PA11Flick, NULL, &PA11Flick_Task_attributes);

  /* creation of Test_CPU_usage */
  Test_CPU_usageHandle = osThreadNew(T_CPU_U_Task, NULL, &Test_CPU_usage_attributes);

  /* creation of PS_Task */
  PS_TaskHandle = osThreadNew(vIP_PS_IJ_Task, NULL, &PS_Task_attributes);

  /* creation of UR_Task */
  UR_TaskHandle = osThreadNew(vIP_UR_Task, NULL, &UR_Task_attributes);

  /* creation of CLI_Task */
  CLI_TaskHandle = osThreadNew(vIP_CLI_Task, NULL, &CLI_Task_attributes);

  /* creation of PC13_Task */
  PC13_TaskHandle = osThreadNew(vIP_PC13_StartTask, NULL, &PC13_Task_attributes);

  /* creation of ERR_Task */
  ERR_TaskHandle = osThreadNew(ERR_Printf_Task, NULL, &ERR_Task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_vIP_KEYTask */
/**
  * @brief  Function implementing the KEY_Task thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_vIP_KEYTask */
__weak void vIP_KEYTask(void *argument)
{
  /* USER CODE BEGIN vIP_KEYTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END vIP_KEYTask */
}

/* USER CODE BEGIN Header_vIP_LEDTask */
/**
* @brief Function implementing the LED_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_vIP_LEDTask */
__weak void vIP_LEDTask(void *argument)
{
  /* USER CODE BEGIN vIP_LEDTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END vIP_LEDTask */
}

/* USER CODE BEGIN Header_vIP_USARTTask */
/**
* @brief Function implementing the USART_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_vIP_USARTTask */
__weak void vIP_USARTTask(void *argument)
{
  /* USER CODE BEGIN vIP_USARTTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END vIP_USARTTask */
}

/* USER CODE BEGIN Header_vIP_OLEDTask */
/**
* @brief Function implementing the OLED_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_vIP_OLEDTask */
__weak void vIP_OLEDTask(void *argument)
{
  /* USER CODE BEGIN vIP_OLEDTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END vIP_OLEDTask */
}

/* USER CODE BEGIN Header_vIP_W25Q64Task */
/**
* @brief Function implementing the W25Q64_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_vIP_W25Q64Task */
__weak void vIP_W25Q64Task(void *argument)
{
  /* USER CODE BEGIN vIP_W25Q64Task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END vIP_W25Q64Task */
}

/* USER CODE BEGIN Header_vIP_PA11Flick */
/**
* @brief Function implementing the PA11Flick_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_vIP_PA11Flick */
__weak void vIP_PA11Flick(void *argument)
{
  /* USER CODE BEGIN vIP_PA11Flick */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END vIP_PA11Flick */
}

/* USER CODE BEGIN Header_T_CPU_U_Task */
/**
* @brief Function implementing the Test_CPU_usage thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_T_CPU_U_Task */
__weak void T_CPU_U_Task(void *argument)
{
  /* USER CODE BEGIN T_CPU_U_Task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END T_CPU_U_Task */
}

/* USER CODE BEGIN Header_vIP_PS_IJ_Task */
/**
* @brief Function implementing the PS_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_vIP_PS_IJ_Task */
__weak void vIP_PS_IJ_Task(void *argument)
{
  /* USER CODE BEGIN vIP_PS_IJ_Task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END vIP_PS_IJ_Task */
}

/* USER CODE BEGIN Header_vIP_UR_Task */
/**
* @brief Function implementing the UR_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_vIP_UR_Task */
__weak void vIP_UR_Task(void *argument)
{
  /* USER CODE BEGIN vIP_UR_Task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END vIP_UR_Task */
}

/* USER CODE BEGIN Header_vIP_CLI_Task */
/**
* @brief Function implementing the CLI_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_vIP_CLI_Task */
__weak void vIP_CLI_Task(void *argument)
{
  /* USER CODE BEGIN vIP_CLI_Task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END vIP_CLI_Task */
}

/* USER CODE BEGIN Header_vIP_PC13_StartTask */
/**
* @brief Function implementing the PC13_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_vIP_PC13_StartTask */
__weak void vIP_PC13_StartTask(void *argument)
{
  /* USER CODE BEGIN vIP_PC13_StartTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END vIP_PC13_StartTask */
}

/* USER CODE BEGIN Header_ERR_Printf_Task */
/**
* @brief Function implementing the ERR_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_ERR_Printf_Task */
__weak void ERR_Printf_Task(void *argument)
{
  /* USER CODE BEGIN ERR_Printf_Task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END ERR_Printf_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

