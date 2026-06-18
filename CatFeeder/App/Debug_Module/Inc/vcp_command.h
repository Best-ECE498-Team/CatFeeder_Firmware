/******************************************************************************
 * @file    vcp_command.h
 * @brief   Virtual COM Port command interface.
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

#ifndef DEBUG_MODULE_INC_VCP_COMMAND_H_
#define DEBUG_MODULE_INC_VCP_COMMAND_H_

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
void VcpCommand_HandleReceived(void);

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_MODULE_INC_VCP_COMMAND_H_ */
