/******************************************************************************
 * @file    vcp_port.h
 * @brief   VCP transport interface over the UART DMA port.
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

#ifndef DEBUG_MODULE_INC_VCP_PORT_H_
#define DEBUG_MODULE_INC_VCP_PORT_H_

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * Includes
 *============================================================================*/
#include <stdbool.h>
#include <stdint.h>

/*=============================================================================
 * Public Function Prototypes
 *============================================================================*/
bool VcpPort_Init(void);
bool VcpPort_WaitForRxEvent(uint32_t ulTimeout_);
uint16_t VcpPort_Read(uint8_t *paucData_, uint16_t usLen_);
uint16_t VcpPort_Write(const uint8_t *paucData_, uint16_t usLen_);
uint16_t VcpPort_WriteString(const char *pcString_);

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_MODULE_INC_VCP_PORT_H_ */
