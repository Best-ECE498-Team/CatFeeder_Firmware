/******************************************************************************
 * @file    ir_thermometer_driver.h
 * @brief
 *
 * @project PawPlate - Intelligent Wet Cat Food Dispensing System
 * @course  University of Waterloo ECE498 Engineering Design Project
 * @team    Team 53
 * @authors Bowen Zheng
 *
 * @license MIT
 * Copyright (c) 2026 Team 53
 *
 * SPDX-License-Identifier: MIT
 *
 ******************************************************************************/

#ifndef THERMAL_MODULE_INC_IR_THERMOMETER_DRIVER_H_
#define THERMAL_MODULE_INC_IR_THERMOMETER_DRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * Includes
 *============================================================================*/
#include <stdint.h>

/*=============================================================================
 * Public Macros
 *============================================================================*/
// Invalid IR temperature reading. This is below the MLX90614 operating range.
#define IR_THERMOMETER_INVALID_TEMPERATURE_C  (-1000.0f)

/*=============================================================================
 * Public Type Definitions
 *============================================================================*/
typedef enum
{
  IR_THERMOMETER_HEATER = 0U,
  IR_THERMOMETER_COOLER1,
  IR_THERMOMETER_COOLER2,
  IR_THERMOMETER_COUNT
}IrThermometerIdTypeDef;

/*=============================================================================
 * Public Constants
 *============================================================================*/


/*=============================================================================
 * Public Function Prototypes
 *============================================================================*/
void IrThermometersInit(void);
float GetIrThermometerObjectTemperatureC(IrThermometerIdTypeDef eIrThermometer_);
float GetIrThermometerAmbientTemperatureC(IrThermometerIdTypeDef eIrThermometer_);

#ifdef __cplusplus
}
#endif

#endif /* THERMAL_MODULE_INC_IR_THERMOMETER_DRIVER_H_ */
