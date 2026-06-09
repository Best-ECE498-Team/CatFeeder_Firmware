/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
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
/* Definitions for SystemTask */
osThreadId_t SystemTaskHandle;
const osThreadAttr_t SystemTask_attributes = {
  .name = "SystemTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 512 * 4
};
/* Definitions for PiCommTask */
osThreadId_t PiCommTaskHandle;
const osThreadAttr_t PiCommTask_attributes = {
  .name = "PiCommTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 512 * 4
};
/* Definitions for ThermalTask */
osThreadId_t ThermalTaskHandle;
const osThreadAttr_t ThermalTask_attributes = {
  .name = "ThermalTask",
  .priority = (osPriority_t) osPriorityAboveNormal,
  .stack_size = 512 * 4
};
/* Definitions for FeedingTask */
osThreadId_t FeedingTaskHandle;
const osThreadAttr_t FeedingTask_attributes = {
  .name = "FeedingTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 512 * 4
};
/* Definitions for DebugCommTask */
osThreadId_t DebugCommTaskHandle;
const osThreadAttr_t DebugCommTask_attributes = {
  .name = "DebugCommTask",
  .priority = (osPriority_t) osPriorityLow,
  .stack_size = 1024 * 4
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartSystemTask(void *argument);
void StartPiCommTask(void *argument);
void StartThermalTask(void *argument);
void StartFeedingTask(void *argument);
void StartDebugCommTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);
void vApplicationMallocFailedHook(void);

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
   /* Run time stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a stack overflow is detected. */
}
/* USER CODE END 4 */

/* USER CODE BEGIN 5 */
void vApplicationMallocFailedHook(void)
{
   /* vApplicationMallocFailedHook() will only be called if
   configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h. It is a hook
   function that will get called if a call to pvPortMalloc() fails.
   pvPortMalloc() is called internally by the kernel whenever a task, queue,
   timer or semaphore is created. It is also called by various parts of the
   demo application. If heap_1.c or heap_2.c are used, then the size of the
   heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
   FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
   to query the size of free heap space that remains (although it does not
   provide information on how the remaining heap might be fragmented). */
}
/* USER CODE END 5 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of SystemTask */
  SystemTaskHandle = osThreadNew(StartSystemTask, NULL, &SystemTask_attributes);

  /* creation of PiCommTask */
  PiCommTaskHandle = osThreadNew(StartPiCommTask, NULL, &PiCommTask_attributes);

  /* creation of ThermalTask */
  ThermalTaskHandle = osThreadNew(StartThermalTask, NULL, &ThermalTask_attributes);

  /* creation of FeedingTask */
  FeedingTaskHandle = osThreadNew(StartFeedingTask, NULL, &FeedingTask_attributes);

  /* creation of DebugCommTask */
  DebugCommTaskHandle = osThreadNew(StartDebugCommTask, NULL, &DebugCommTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartSystemTask */
/**
* @brief Function implementing the SystemTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSystemTask */
__weak void StartSystemTask(void *argument)
{
  /* USER CODE BEGIN StartSystemTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartSystemTask */
}

/* USER CODE BEGIN Header_StartPiCommTask */
/**
* @brief Function implementing the PiCommTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartPiCommTask */
__weak void StartPiCommTask(void *argument)
{
  /* USER CODE BEGIN StartPiCommTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartPiCommTask */
}

/* USER CODE BEGIN Header_StartThermalTask */
/**
* @brief Function implementing the ThermalTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartThermalTask */
__weak void StartThermalTask(void *argument)
{
  /* USER CODE BEGIN StartThermalTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartThermalTask */
}

/* USER CODE BEGIN Header_StartFeedingTask */
/**
* @brief Function implementing the FeedingTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartFeedingTask */
__weak void StartFeedingTask(void *argument)
{
  /* USER CODE BEGIN StartFeedingTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartFeedingTask */
}

/* USER CODE BEGIN Header_StartDebugCommTask */
/**
* @brief Function implementing the DebugCommTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDebugCommTask */
__weak void StartDebugCommTask(void *argument)
{
  /* USER CODE BEGIN StartDebugCommTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDebugCommTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

