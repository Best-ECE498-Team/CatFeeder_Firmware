/******************************************************************************
 * @file    vcp_types.h
 * @brief   VCP command parsing macros and structures.
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

#ifndef DEBUG_MODULE_INC_VCP_TYPES_H_
#define DEBUG_MODULE_INC_VCP_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * Includes
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
/*=============================================================================
 * Public Macros
 *============================================================================*/
#define VCP_DEBUG_CMD_MAX_ARGS    (8U) /**< Maximum number of arguments in a debug command */

/*=============================================================================
 * Public Type Definitions
 *============================================================================*/
/**
 * @brief Structure of a parsed VCP command.
 */
typedef struct
{
  // Command format: <module>.<action> [param=value] ...
  // Command example: cf debug.set level=error mask=system,comm
  char *pcModule;                           /**< Module name for command filtering */
  char *pcAction;                           /**< Command action */
  uint8_t ucArgc;                           /**< Number of arguments */
  char *pacArgv[VCP_DEBUG_CMD_MAX_ARGS];    /**< Argument strings */
} VcpCommandTypeDef;

// Define pointer to command handler type and module registry entry definitions
typedef bool (*VcpCmdHandlerTypeDef)(const VcpCommandTypeDef *pstCmd_);

/**
 * @brief Module registry entry for VCP command dispatching.
 */
 typedef struct
{
  const char *pcModule;             /**< Module name used for command dispatching */           
  VcpCmdHandlerTypeDef pfHandler;   /**< Command handler function pointer */
} VcpCommandModuleEntryTypeDef;

/*=============================================================================
 * Public Inline Functions
 *============================================================================*/
/**
 * @brief Compare a command token with an expected string.
 *
 * @param[in] pcText_     Token text.
 * @param[in] pcExpected_ Expected token text.
 *
 * @return true when equal, false otherwise.
 */
static inline bool IsVcpTokenEqual(const char *pcText_, const char *pcExpected_)
{
  // Reject missing comparison inputs.
  if ((pcText_ == NULL) || (pcExpected_ == NULL))
    return false;

  // Compare until either token ends.
  while ((*pcText_ != '\0') && (*pcText_ != '\r') && (*pcExpected_ != '\0'))
  {
    // Stop on the first mismatch.
    if (*pcText_ != *pcExpected_)
      return false;

    pcText_++;
    pcExpected_++;
  }

  return (((*pcText_ == '\0') || (*pcText_ == '\r')) && (*pcExpected_ == '\0'));
}

/**
 * @brief Check whether an argument token has the expected key.
 *
 * @param[in] pcArg_ Argument token in key=value form.
 * @param[in] pcKey_ Expected key without the equals sign.
 *
 * @return true when the argument key matches, false otherwise.
 */
static inline bool IsVcpArgKeyEqual(const char *pcArg_, const char *pcKey_)
{
  // Reject missing comparison inputs.
  if ((pcArg_ == NULL) || (pcKey_ == NULL))
    return false;

  // Compare the key portion before the equals sign.
  while ((*pcArg_ != '\0') && (*pcArg_ != '\r') && (*pcArg_ != '=') && (*pcKey_ != '\0'))
  {
    // Stop on the first key mismatch.
    if (*pcArg_ != *pcKey_)
      return false;

    pcArg_++;
    pcKey_++;
  }

  return ((*pcArg_ == '=') && (*pcKey_ == '\0'));
}

/*=============================================================================
 * Public Constants
 *============================================================================*/


/*=============================================================================
 * Public Function Prototypes
 *============================================================================*/

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_MODULE_INC_VCP_TYPES_H_ */
