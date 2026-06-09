/******************************************************************************
 * @file    vcp_debug.h
 * @brief   Header for vcp_debug.h file
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

#ifndef COMMUNICATION_MODULE_INC_VCP_DEBUG_H_
#define COMMUNICATION_MODULE_INC_VCP_DEBUG_H_

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * Includes
 *============================================================================*/
#include "main.h"

/*=============================================================================
 * Public Macros
 *============================================================================*/
#define DPRINTF_ERROR(mask_, fmt_, ...)    VcpDebugPrintf(LEVEL_ERROR,   (mask_), (fmt_), ##__VA_ARGS__)
#define DPRINTF_WARN(mask_, fmt_, ...)     VcpDebugPrintf(LEVEL_WARN,    (mask_), (fmt_), ##__VA_ARGS__)
#define DPRINTF_INFO(mask_, fmt_, ...)     VcpDebugPrintf(LEVEL_INFO,    (mask_), (fmt_), ##__VA_ARGS__)

#ifdef DEMO
#define DPRINTF_DEBUG(mask_, fmt_, ...)    ((void)0)
#define DPRINTF_VERBOSE(mask_, fmt_, ...)  ((void)0)
#else
#define DPRINTF_DEBUG(mask_, fmt_, ...)    VcpDebugPrintf(LEVEL_DEBUG,   (mask_), (fmt_), ##__VA_ARGS__)
#define DPRINTF_TRACE(mask_, fmt_, ...)    VcpDebugPrintf(LEVEL_TRACE, (mask_), (fmt_), ##__VA_ARGS__)
#endif

// Debug target mask definitions
#define DBG_MASK_NONE     (0U)
#define DBG_MASK_SYSTEM   (1UL << 0)
#define DBG_MASK_COMM     (1UL << 1)
#define DBG_MASK_THERMAL  (1UL << 2)
#define DBG_MASK_FEEDING  (1UL << 3)
#define VCP_DBG_MASK_ALL  (0xFFFFFFFFUL)

/*=============================================================================
 * Public Type Definitions
 *============================================================================*/
typedef enum
{
  LEVEL_ERROR   = 0U,    /**< Fatal errors, faults */
  LEVEL_WARN    = 1U,    /**< Abnormal but recoverable conditions */
  LEVEL_INFO    = 2U,    /**< Normal operational events */
  LEVEL_DEBUG   = 3U,    /**< Detailed debugging information */
  LEVEL_TRACE   = 4U     /**< Very high frequency diagnostic output */
} VcpDebugLevelTypedef;

/*=============================================================================
 * Public Constants
 *============================================================================*/


/*=============================================================================
 * Public Function Prototypes
 *============================================================================*/
void VcpDebug_Init(void);
void VcpDebug_SetLevel(VcpDebugLevelTypedef eLevel_);
void VcpDebug_SetTargetMask(uint32_t ulTargetMask_);
void VcpDebugPrintf(VcpDebugLevelTypedef eLevel_, uint32_t ulTargetMask_, const char* pcFormat_, ...);

#ifdef __cplusplus
}
#endif

#endif /* COMMUNICATION_MODULE_INC_VCP_DEBUG_H_ */
