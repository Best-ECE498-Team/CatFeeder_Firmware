/******************************************************************************
 * @file    system_vcp_command.h
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

#ifndef SYSTEM_MANAGER_INC_SYSTEM_VCP_COMMAND_H_
#define SYSTEM_MANAGER_INC_SYSTEM_VCP_COMMAND_H_

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * Includes
 *============================================================================*/
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
bool HandleSystemVcpCommand(const VcpCommandTypeDef *pstCmd_);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_MANAGER_INC_SYSTEM_VCP_COMMAND_H_ */
