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
#include "../Internal/vcp_command.h"
#include "vcp_debug.h"
#include "../Internal/vcp_port.h"
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
  (void)argument;
  
  // Initialize the VCP UART DMA transport
  VcpDebug_Init();
  DPRINTF_VCP("VCP DEBUG PORT Initialized\r\n");
  
  /* Infinite loop */
  for(;;)
  {
    // Wait for a VCP RX event and handle received commands
    if (VcpPort_WaitForRxEvent(osWaitForever) != false)
      VcpCommand_HandleReceived();

  }

}
