/******************************************************************************
 * @file    thermal_vcp_command.h
 * @brief
 *
 * @project PawPlate - Intelligent Wet Cat Food Dispensing System
 * @course  University of Waterloo ECE498 Engineering Design Project
 * @team    Team 53
 * @authors
 *
 * @license MIT
 * Copyright (c) 2026 Team 53
 *
 * SPDX-License-Identifier: MIT
 *
 ******************************************************************************/

#ifndef THERMAL_MODULE_INC_THERMAL_VCP_COMMAND_H_
#define THERMAL_MODULE_INC_THERMAL_VCP_COMMAND_H_

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * Includes
 *============================================================================*/
#include <stdbool.h>  
#include "vcp_types.h"

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
bool HandleThermalVcpCommand(const VcpCommandTypeDef *pstCmd_);

#ifdef __cplusplus
}
#endif

#endif /* THERMAL_MODULE_INC_THERMAL_VCP_COMMAND_H_ */
