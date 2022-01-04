/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "app_main/app_main.h"
#include "app_config.h"
#include "app_lte/lteTask.h"
#include "app_mqtt/mqttTask.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
typedef StaticQueue_t osStaticMessageQDef_t;
typedef StaticSemaphore_t osStaticMutexDef_t;
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
/* Definitions for mainTask */
osThreadId_t mainTaskHandle;
uint32_t mainTaskBuffer[ 384 ];
osStaticThreadDef_t mainTaskControlBlock;
const osThreadAttr_t mainTask_attributes = {
  .name = "mainTask",
  .cb_mem = &mainTaskControlBlock,
  .cb_size = sizeof(mainTaskControlBlock),
  .stack_mem = &mainTaskBuffer[0],
  .stack_size = sizeof(mainTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for lteTask */
osThreadId_t lteTaskHandle;
uint32_t lteTaskBuffer[ 384 ];
osStaticThreadDef_t lteTaskControlBlock;
const osThreadAttr_t lteTask_attributes = {
  .name = "lteTask",
  .cb_mem = &lteTaskControlBlock,
  .cb_size = sizeof(lteTaskControlBlock),
  .stack_mem = &lteTaskBuffer[0],
  .stack_size = sizeof(lteTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for mqttTask */
osThreadId_t mqttTaskHandle;
uint32_t mqttTaskBuffer[ 384 ];
osStaticThreadDef_t mqttTaskControlBlock;
const osThreadAttr_t mqttTask_attributes = {
  .name = "mqttTask",
  .cb_mem = &mqttTaskControlBlock,
  .cb_size = sizeof(mqttTaskControlBlock),
  .stack_mem = &mqttTaskBuffer[0],
  .stack_size = sizeof(mqttTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for mainMail */
osMessageQueueId_t mainMailHandle;
uint8_t mainMailBuffer[ 5 * sizeof( mail_t ) ];
osStaticMessageQDef_t mainMailControlBlock;
const osMessageQueueAttr_t mainMail_attributes = {
  .name = "mainMail",
  .cb_mem = &mainMailControlBlock,
  .cb_size = sizeof(mainMailControlBlock),
  .mq_mem = &mainMailBuffer,
  .mq_size = sizeof(mainMailBuffer)
};
/* Definitions for lteMail */
osMessageQueueId_t lteMailHandle;
uint8_t lteMailBuffer[ 5 * sizeof( mail_t ) ];
osStaticMessageQDef_t lteMailControlBlock;
const osMessageQueueAttr_t lteMail_attributes = {
  .name = "lteMail",
  .cb_mem = &lteMailControlBlock,
  .cb_size = sizeof(lteMailControlBlock),
  .mq_mem = &lteMailBuffer,
  .mq_size = sizeof(lteMailBuffer)
};
/* Definitions for mqttMail */
osMessageQueueId_t mqttMailHandle;
uint8_t mqttMailBuffer[ 5 * sizeof( mail_t ) ];
osStaticMessageQDef_t mqttMailControlBlock;
const osMessageQueueAttr_t mqttMail_attributes = {
  .name = "mqttMail",
  .cb_mem = &mqttMailControlBlock,
  .cb_size = sizeof(mqttMailControlBlock),
  .mq_mem = &mqttMailBuffer,
  .mq_size = sizeof(mqttMailBuffer)
};
/* Definitions for appResource */
osMutexId_t appResourceHandle;
osStaticMutexDef_t appResourceControlBlock;
const osMutexAttr_t appResource_attributes = {
  .name = "appResource",
  .cb_mem = &appResourceControlBlock,
  .cb_size = sizeof(appResourceControlBlock),
};
/* Definitions for debugMutex */
osMutexId_t debugMutexHandle;
osStaticMutexDef_t debugMutexControlBlock;
const osMutexAttr_t debugMutex_attributes = {
  .name = "debugMutex",
  .cb_mem = &debugMutexControlBlock,
  .cb_size = sizeof(debugMutexControlBlock),
};
/* Definitions for buzzerMutex */
osMutexId_t buzzerMutexHandle;
osStaticMutexDef_t buzzerMutexControlBlock;
const osMutexAttr_t buzzerMutex_attributes = {
  .name = "buzzerMutex",
  .cb_mem = &buzzerMutexControlBlock,
  .cb_size = sizeof(buzzerMutexControlBlock),
};
/* Definitions for flashMutex */
osMutexId_t flashMutexHandle;
osStaticMutexDef_t flashMutexControlBlock;
const osMutexAttr_t flashMutex_attributes = {
  .name = "flashMutex",
  .cb_mem = &flashMutexControlBlock,
  .cb_size = sizeof(flashMutexControlBlock),
};
/* Definitions for lteRingbufMutex */
osMutexId_t lteRingbufMutexHandle;
osStaticMutexDef_t lteRingbufMutexControlBlock;
const osMutexAttr_t lteRingbufMutex_attributes = {
  .name = "lteRingbufMutex",
  .cb_mem = &lteRingbufMutexControlBlock,
  .cb_size = sizeof(lteRingbufMutexControlBlock),
};

/* Definitions for jigtestMutex */
osMutexId_t jigtestMutexHandle;
osStaticMutexDef_t jigtestMutexControlBlock;
const osMutexAttr_t jigtestMutex_attributes = {
  .name = "jigtestMutex",
  .cb_mem = &jigtestMutexControlBlock,
  .cb_size = sizeof(jigtestMutexControlBlock),
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartMainTask(void *argument);
void LteTaskStart(void *argument);
void MqttTaskStart(void *argument);

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
  /* creation of appResource */
  appResourceHandle = osMutexNew(&appResource_attributes);

  /* creation of debugMutex */
  debugMutexHandle = osMutexNew(&debugMutex_attributes);

  /* creation of buzzerMutex */
  buzzerMutexHandle = osMutexNew(&buzzerMutex_attributes);

  /* creation of flashMutex */
  flashMutexHandle = osMutexNew(&flashMutex_attributes);

  /* creation of lteRingbufMutex */
  lteRingbufMutexHandle = osMutexNew(&lteRingbufMutex_attributes);

  jigtestMutexHandle = osMutexNew(&jigtestMutex_attributes);
  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of mainMail */
  mainMailHandle = osMessageQueueNew (5, sizeof(mail_t), &mainMail_attributes);

  /* creation of lteMail */
  lteMailHandle = osMessageQueueNew (5, sizeof(mail_t), &lteMail_attributes);

  /* creation of mqttMail */
  mqttMailHandle = osMessageQueueNew (5, sizeof(mail_t), &mqttMail_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of mainTask */
  mainTaskHandle = osThreadNew(StartMainTask, NULL, &mainTask_attributes);

  /* creation of lteTask */
  lteTaskHandle = osThreadNew(LteTaskStart, NULL, &lteTask_attributes);

  /* creation of mqttTask */
  mqttTaskHandle = osThreadNew(MqttTaskStart, NULL, &mqttTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */ /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartMainTask */
/**
  * @brief  Function implementing the mainTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartMainTask */
void StartMainTask(void *argument)
{
  /* USER CODE BEGIN StartMainTask */
  /* Infinite loop */
	app_main();
  for(;;)
  {
    osDelay(5);
  }
  /* USER CODE END StartMainTask */
}

/* USER CODE BEGIN Header_LteTaskStart */
/**
* @brief Function implementing the lteTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_LteTaskStart */
void LteTaskStart(void *argument)
{
  /* USER CODE BEGIN LteTaskStart */
  /* Infinite loop */
#ifdef JIGTEST
	jigtest_lte_task();
#else
	lte_task();
#endif
  for(;;)
  {
    osDelay(5);
  }
  /* USER CODE END LteTaskStart */
}

/* USER CODE BEGIN Header_MqttTaskStart */
/**
* @brief Function implementing the mqttTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_MqttTaskStart */
void MqttTaskStart(void *argument)
{
  /* USER CODE BEGIN MqttTaskStart */
  /* Infinite loop */
	app_mqtt();
  for(;;)
  {
    osDelay(5);
  }
  /* USER CODE END MqttTaskStart */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
