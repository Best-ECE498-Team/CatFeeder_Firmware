/******************************************************************************
 * @file    thermal_task.c
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
#include "thermal_module.h"
#include "cmsis_os.h"
#include "stm32g4xx_hal.h"
#include "vcp_debug.h"
#include "..\internal\thermal_adc_driver.h"

/*=============================================================================
 * Private Macros
 *============================================================================*/


/*=============================================================================
 * Private Type Definitions
 *============================================================================*/


/*=============================================================================
 * Private Variables
 *============================================================================*/
uint16_t au16AdcBuffer[7] = {0U};

/*=============================================================================
 * Private Function Prototypes
 *============================================================================*/
void StartThermalTask(void *argument);

/*=============================================================================
 * Public Function Definitions
 *============================================================================*/

/**
 * @brief
 *
 * @details
 *
 * @param[in]
 * @param[out]
 *
 * @return
 */


/*=============================================================================
 * Private Function Definitions
 *============================================================================*/

/**
 * @brief  Function implementing the ThermalTask thread.
 *
 * @param[in]  None
 * @param[out] None
 *
 * @return None
 */
void StartThermalTask(void *argument)
{
  UNUSED(argument);
  
  if (StartThermalAdcSampling() == false)
  {
    DPRINTF_ERROR(DBG_MASK_THERMAL, "THERMAL_ADC_START_FAIL\r\n");
    // Handle thermal ADC failure (TODO)
  }
  
  IrThermometersInit();

  /* Infinite loop */
  for(;;)
  {
    //PrintThermalAdcValues();
    osDelay(100);
  }
}
