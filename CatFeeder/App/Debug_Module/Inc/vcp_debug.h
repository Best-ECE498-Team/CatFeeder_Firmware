/******************************************************************************
 * @file    vcp_debug.h
 * @brief   Header for vcp_debug.h file
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

#ifndef COMMUNICATION_MODULE_INC_VCP_DEBUG_H_
#define COMMUNICATION_MODULE_INC_VCP_DEBUG_H_

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
#define DPRINTF_ERROR(mask_, fmt_, ...)  \
  VcpDebugPrintf(LEVEL_ERROR,   (mask_), (fmt_), ##__VA_ARGS__)
#define DPRINTF_WARN(mask_, fmt_, ...)   \
  VcpDebugPrintf(LEVEL_WARN,    (mask_), (fmt_), ##__VA_ARGS__)
#define DPRINTF_INFO(mask_, fmt_, ...)   \
  VcpDebugPrintf(LEVEL_INFO,    (mask_), (fmt_), ##__VA_ARGS__)
#define DPRINTF_VCP(fmt_, ...)           \
  VcpDebugPrintf(LEVEL_INFO, DBG_MASK_VCP, (fmt_), ##__VA_ARGS__)
#ifndef DEBUG
#define DPRINTF_DEBUG(mask_, fmt_, ...)  ((void)0)
#define DPRINTF_TRACE(mask_, fmt_, ...)  ((void)0)
#else
#define DPRINTF_DEBUG(mask_, fmt_, ...)  \
  VcpDebugPrintf(LEVEL_DEBUG,   (mask_), (fmt_), ##__VA_ARGS__)
#define DPRINTF_TRACE(mask_, fmt_, ...)  \
  VcpDebugPrintf(LEVEL_TRACE, (mask_), (fmt_), ##__VA_ARGS__)
#endif

// Debug target mask definitions
#define DBG_MASK_NONE     (0U)
#define DBG_MASK_SYSTEM   (1UL << 0)
#define DBG_MASK_COMM     (1UL << 1)
#define DBG_MASK_THERMAL  (1UL << 2)
#define DBG_MASK_FEEDING  (1UL << 3)
#define DBG_MASK_VCP      (1UL << 31)
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
} VcpDebugLevelTypeDef;

/*=============================================================================
 * Public Constants
 *============================================================================*/


/*=============================================================================
 * Public Function Prototypes
 *============================================================================*/
void VcpDebug_Init(void);
void VcpDebugPrintf(VcpDebugLevelTypeDef eLevel_, uint32_t ulTargetMask_, const char* pcFormat_, ...);
bool HandleDebugVcpCommand(const VcpCommandTypeDef *pstCmd_);

#ifdef __cplusplus
}
#endif

#endif /* COMMUNICATION_MODULE_INC_VCP_DEBUG_H_ */
