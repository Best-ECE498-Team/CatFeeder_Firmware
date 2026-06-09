/******************************************************************************
 * @file    comm_debug_task.c
 * @brief   
 *
 * @project PawPlate - Intelligent Wet Cat Food Dispensing System
 * @course  ECE 498 Engineering Design Project
 * @team    Team 53
 * @authors 
 *
 * @license MIT
 * Copyright (c) 2026 Team 53
 *
 * SPDX-License-Identifier: MIT
 *
 ******************************************************************************/

/*=============================================================================
 * Includes
 *============================================================================*/
#include "vcp_debug.h"
#include "cmsis_os.h"

/*=============================================================================
 * Private Macros
 *============================================================================*/


/*=============================================================================
 * Private Type Definitions
 *============================================================================*/


/*=============================================================================
 * Private Variables
 *============================================================================*/


/*=============================================================================
 * Private Function Prototypes
 *============================================================================*/
void StartDebugCommTask(void *argument);

/*=============================================================================
 * Private Function Definitions
 *============================================================================*/

/**
 * @brief Function implementing the DebugCommTask thread.
 *
 * @param[in]  Not used
 * @param[out] Not used
 *
 * @return Not used
 */
void StartDebugCommTask(void *argument)
{
  /* USER CODE BEGIN StartDebugCommTask */
  VcpDebug_Init();
  uint32_t ulCount = 0;
  /* Infinite loop */
  for(;;)
  {
    ulCount += 1;
    DPRINTF_ERROR(DBG_MASK_FEEDING, "Error From DebugCommTask Count: %u\r\n", ulCount);
    DPRINTF_WARN(DBG_MASK_COMM, "Warn From DebugCommTask Count: %u\r\n", ulCount);
    DPRINTF_INFO(DBG_MASK_SYSTEM, "Info From DebugCommTask Count: %u\r\n", ulCount);
    DPRINTF_DEBUG(DBG_MASK_THERMAL, "Debug From DebugCommTask Count: %u\r\n", ulCount);
    DPRINTF_TRACE(DBG_MASK_SYSTEM, "Trace From DebugCommTask Count: %u\r\n", ulCount);
    osDelay(100);
  }
  /* USER CODE END StartDebugCommTask */
}
