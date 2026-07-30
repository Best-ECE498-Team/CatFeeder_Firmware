/******************************************************************************
 * @file    vcp_command.c
 * @brief   Virtual COM Port command implementation.
 *
 * @project PawPlate - Intelligent Wet Cat Food Dispensing System
 * @course  ECE 498 Engineering Design Project
 * @team    Team 53
 * @authors Bowen Zheng
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
#include "../Internal/vcp_command.h"
#include "system_vcp_command.h"
#include "vcp_debug.h"
#include "../Internal/vcp_port.h"
#include <string.h>
#include "thermal_vcp_command.h"
/*=============================================================================
 * Private Macros
 *============================================================================*/
#define VCP_COMMAND_PREFIX  "cf"
#define VCP_COMMAND_BUF_SIZE  (256U)

/*=============================================================================
 * Private Type Definitions
 *============================================================================*/
static const VcpCommandModuleEntryTypeDef astVcpCmdModuleTable[] =
{
  { "debug",   HandleDebugVcpCommand  },
  { "system",  HandleSystemVcpCommand },
  { "thermal", HandleThermalVcpCommand },
  // { "feeding", HandleFeedingVcpCommand },
  // { "comm",    HandleCommVcpCommand }
};

/*=============================================================================
 * Private Variables
 *============================================================================*/
static char aucTheVcpCommandBuf[VCP_COMMAND_BUF_SIZE];
static uint16_t usTheVcpCommandIndex;

/*=============================================================================
 * Private Function Prototypes
 *============================================================================*/
static char* TrimVcpCommandToken(char *pcText_);
static bool VcpCommand_Parse(char *pcLine_, VcpCommandTypeDef *pstCmd_);
static bool VcpCommand_Dispatch(const VcpCommandTypeDef *pstCmd_);
static bool VcpCommand_ProcessLine(char *pcLine_);

/*=============================================================================
 * Public Function Definitions
 *============================================================================*/
/**
 * @brief Consume received VCP bytes and process complete command lines.
 */
void VcpCommand_HandleReceived(void)
{
  uint8_t ucByte = 0U;

  // Consume all bytes currently available from the RX ring.
  while (VcpPort_Read(&ucByte, 1U) > 0U)
  {
    // Treat NUL, CR, and LF as command terminators.
    if ((ucByte == '\0') || (ucByte == '\r') || (ucByte == '\n'))
    {
      if (usTheVcpCommandIndex > 0U)
      {
        aucTheVcpCommandBuf[usTheVcpCommandIndex] = '\0';
        if (VcpCommand_ProcessLine(aucTheVcpCommandBuf) == false)
          DPRINTF_VCP("ERR unknown command\r\n");

        usTheVcpCommandIndex = 0U;
      }

      continue;
    }

    // Drop an overlong command and restart accumulation.
    if (usTheVcpCommandIndex >= (VCP_COMMAND_BUF_SIZE - 1U))
    {
      usTheVcpCommandIndex = 0U;
      DPRINTF_VCP("ERR command too long\r\n");
      continue;
    }

    aucTheVcpCommandBuf[usTheVcpCommandIndex] = (char)ucByte;
    usTheVcpCommandIndex++;
  }
}

/*=============================================================================
 * Private Function Definitions
 *============================================================================*/
/**
 * @brief Parse one VCP command line into a command structure.
 *
 * @details The input line is modified in place by inserting string terminators.
 *
 * @param[in,out] pcLine_  Command line to parse.
 * @param[out]    pstCmd_  Parsed command structure.
 *
 * @return true if parsing succeeded, false otherwise.
 */
static bool VcpCommand_Parse(char *pcLine_, VcpCommandTypeDef *pstCmd_)
{
  char *pcCursor = NULL;
  char *pcToken = NULL;
  char *pcModuleAction = NULL;
  char *pcDot = NULL;

  // Reject missing parser inputs.
  if ((pcLine_ == NULL) || (pstCmd_ == NULL))
    return false;

  (void)memset(pstCmd_, 0, sizeof(*pstCmd_));

  pcCursor = TrimVcpCommandToken(pcLine_);
  // Reject empty command lines.
  if (*pcCursor == '\0')
    return false;

  // Require the public command prefix.
  if ((strncmp(pcCursor, VCP_COMMAND_PREFIX, strlen(VCP_COMMAND_PREFIX)) != 0) ||
      ((pcCursor[strlen(VCP_COMMAND_PREFIX)] != ' ') &&
       (pcCursor[strlen(VCP_COMMAND_PREFIX)] != '\t') &&
       (pcCursor[strlen(VCP_COMMAND_PREFIX)] != '\0') &&
       (pcCursor[strlen(VCP_COMMAND_PREFIX)] != '\r')))
  {
    return false;
  }

  pcCursor = TrimVcpCommandToken(&pcCursor[strlen(VCP_COMMAND_PREFIX)]);
  // Reject prefix-only commands.
  if (*pcCursor == '\0')
    return false;

  pcModuleAction = pcCursor;
  // Find the end of the module.action token.
  while ((*pcCursor != '\0') && (*pcCursor != ' ') && (*pcCursor != '\t') && (*pcCursor != '\r'))
    pcCursor++;

  // Terminate module.action before parsing arguments.
  if (*pcCursor != '\0')
  {
    *pcCursor = '\0';
    pcCursor++;
  }

  pcDot = strchr(pcModuleAction, '.');
  // Require both module and action names.
  if ((pcDot == NULL) || (pcDot == pcModuleAction) || (pcDot[1] == '\0'))
    return false;

  *pcDot = '\0';
  pstCmd_->pcModule = pcModuleAction;
  pstCmd_->pcAction = pcDot + 1;

  pcCursor = TrimVcpCommandToken(pcCursor);
  // Collect all remaining whitespace-delimited arguments.
  while (*pcCursor != '\0')
  {
    // Enforce the fixed argument storage limit.
    if (pstCmd_->ucArgc >= VCP_DEBUG_CMD_MAX_ARGS)
      return false;

    pcToken = pcCursor;
    // Find the end of this argument token.
    while ((*pcCursor != '\0') && (*pcCursor != ' ') && (*pcCursor != '\t') && (*pcCursor != '\r'))
      pcCursor++;

    // Terminate the argument token if more input follows.
    if (*pcCursor != '\0')
    {
      *pcCursor = '\0';
      pcCursor++;
    }

    pstCmd_->pacArgv[pstCmd_->ucArgc] = pcToken;
    pstCmd_->ucArgc++;
    pcCursor = TrimVcpCommandToken(pcCursor);
  }

  return true;
}

/**
 * @brief Dispatch a parsed VCP command to its module handler.
 *
 * @param[in] pstCmd_ Parsed command.
 *
 * @return true if a module handled the command, false otherwise.
 */
static bool VcpCommand_Dispatch(const VcpCommandTypeDef *pstCmd_)
{
  uint16_t usIndex = 0U;
  uint16_t usModuleCount = (uint16_t)(sizeof(astVcpCmdModuleTable) / sizeof(astVcpCmdModuleTable[0]));

  // Reject incomplete parsed commands.
  if ((pstCmd_ == NULL) || (pstCmd_->pcModule == NULL) || (pstCmd_->pcAction == NULL))
    return false;

  // Search for a registered module handler.
  for (usIndex = 0U; usIndex < usModuleCount; usIndex++)
  {
    // Dispatch through the first matching module entry.
    if ((astVcpCmdModuleTable[usIndex].pcModule != NULL) &&
        (astVcpCmdModuleTable[usIndex].pfHandler != NULL) &&
        (IsVcpTokenEqual(pstCmd_->pcModule, astVcpCmdModuleTable[usIndex].pcModule) != false))
    {
      return astVcpCmdModuleTable[usIndex].pfHandler(pstCmd_);
    }
  }

  return false;
}

/**
 * @brief Parse and dispatch one VCP command line.
 *
 * @param[in,out] pcLine_ Command line, modified during parsing.
 *
 * @return true if the command parsed and was handled, false otherwise.
 */
static bool VcpCommand_ProcessLine(char *pcLine_)
{
  VcpCommandTypeDef stCmd;

  // Stop early if the line is not a valid command.
  if (VcpCommand_Parse(pcLine_, &stCmd) == false)
    return false;

  return VcpCommand_Dispatch(&stCmd);
}

/**
 * @brief Trim leading and trailing command whitespace in place.
 *
 * @param[in,out] pcText_ Text to trim.
 *
 * @return Pointer to first non-whitespace character.
 */
static char* TrimVcpCommandToken(char *pcText_)
{
  char *pcEnd = NULL;

  // Treat null text as an empty token.
  if (pcText_ == NULL)
    return "";

  // Skip leading command whitespace.
  while ((*pcText_ == ' ') || (*pcText_ == '\t') || (*pcText_ == '\r'))
    pcText_++;

  pcEnd = pcText_;
  // Find the string terminator.
  while (*pcEnd != '\0')
    pcEnd++;

  // Remove trailing command whitespace.
  while ((pcEnd > pcText_) &&
         ((pcEnd[-1] == ' ') || (pcEnd[-1] == '\t') || (pcEnd[-1] == '\r')))
  {
    pcEnd--;
    *pcEnd = '\0';
  }

  return pcText_;
}
