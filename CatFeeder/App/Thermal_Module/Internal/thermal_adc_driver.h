/******************************************************************************
 * @file    thermal_adc_driver.h
 * @brief   thermal_adc_driver public APIs
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

#ifndef THERMAL_MODULE_INC_THERMAL_ADC_DRIVER_H_
#define THERMAL_MODULE_INC_THERMAL_ADC_DRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * Includes
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>

/*=============================================================================
 * Public Macros
 *============================================================================*/


/*=============================================================================
 * Public Type Definitions
 *============================================================================*/


/*=============================================================================
 * Public Constants
 *============================================================================*/


/*=============================================================================
 * Public Function Prototypes
 *============================================================================*/
bool StartThermalAdcSampling(void);
void PrintThermalAdcRawValues(void);
void PrintThermalAdcValues(void);
void PrintThermalAdcTemperatures(void);
int16_t GetMcuJunctionTemperatureC(void);
int16_t GetHeaterNtcTemperatureC(void);
int16_t GetCooler1NtcTemperatureC(void);
int16_t GetCooler2NtcTemperatureC(void);
int16_t GetCooler1HeatSinkNtcTemperatureC(void);
int16_t GetCooler2HeatSinkNtcTemperatureC(void);

#ifdef __cplusplus
}
#endif

#endif /* THERMAL_MODULE_INC_THERMAL_ADC_DRIVER_H_ */
