/******************************************************************************
 * @file    system_task.c
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
#include "system_manager.h"
#include "cmsis_os.h"
#include "stm32g4xx_nucleo.h"

/*=============================================================================
 * Private Macros
 *============================================================================*/


/*=============================================================================
 * Private Type Definitions
 *============================================================================*/


/*=============================================================================
 * Private Variables
 *============================================================================*/
extern uint32_t BspButtonState;

/*=============================================================================
 * Private Function Prototypes
 *============================================================================*/
void StartSystemTask(void *argument);

/*=============================================================================
 * Public Function Definitions
 *============================================================================*/

/**
 * @brief Function implementing the SystemTask thread.
 *
 * @param[in] None
 * @param[out] None
 *
 * @return None
 */
void StartSystemTask(void *argument)
{

  /* Infinite loop */
  for(;;)
  {
    /* -- Sample board code for User push-button in interrupt mode ---- */
    if (BspButtonState == BUTTON_PRESSED)
    {
      BspButtonState = BUTTON_RELEASED;
      BSP_LED_Toggle(LED_GREEN);
    }

    osDelay(10);
  }

}

/*=============================================================================
 * Private Function Definitions
 *============================================================================*/

/**
 * @brief
 *
 * @param[in]
 * @param[out]
 *
 * @return
 */
