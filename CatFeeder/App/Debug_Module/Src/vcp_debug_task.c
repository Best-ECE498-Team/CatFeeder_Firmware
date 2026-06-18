/******************************************************************************
 * @file    vcp_debug_task.c
 * @brief   Virtual COM Port debug task implementation.
 *
 * @project PawPlate - Intelligent Wet Cat Food Dispensing System
 * @course  ECE 498 Engineering Design Project
 * @team    Team 53
 * @authors Bowen Zheng
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
#include "vcp_command.h"
#include "vcp_debug.h"
#include "vcp_port.h"
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
  // Initialize VCP port and debug module
  (void)VcpPort_Init();
  VcpDebug_Init();

  /* Infinite loop */
  for(;;)
  {
    // Wait for a VCP RX event and handle received commands
    if (VcpPort_WaitForRxEvent(osWaitForever) != false)
      VcpCommand_HandleReceived();

  }

}
