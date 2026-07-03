/******************************************************************************
 * @file    vcp_debug.c
 * @brief   Virtual com port debugging message transfer and command handling
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
#include "vcp_debug.h"
#include "../Internal/vcp_command.h"
#include "../Internal/vcp_port.h"
#include "stm32g4xx.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/*=============================================================================
 * Private Macros
 *============================================================================*/
#define VCP_DEBUG_PRINTF_BUF_SIZE (256U)

/*=============================================================================
 * Private Type Definitions
 *============================================================================*/

/*=============================================================================
 * Private Variables
 *============================================================================*/
static VcpDebugLevelTypeDef ulTheDebugLevel;
static uint32_t ulTheDebugTargetMask;

/*=============================================================================
 * Private Function Prototypes
 *============================================================================*/
static bool IsVcpDebugPrintfEnabled(VcpDebugLevelTypeDef eLevel_, uint32_t ulTargetMask_);
static const char* GetVcpDebugLevelString(VcpDebugLevelTypeDef eLevel_);
static const char* GetVcpDebugMaskString(uint32_t ulMask_);
static uint16_t VcpDebug_Write(const uint8_t *paucData_, uint16_t usLen_);
static uint16_t VcpDebug_WriteString(const char *pcString_);
static bool ParseVcpDebugLevel(const char *pcText_, VcpDebugLevelTypeDef *peLevel_);
static bool ParseVcpDebugTargetMaskList(const char *pcText_, uint32_t *pulMask_);
static bool ParseVcpDebugTargetMaskName(const char *pcText_, uint16_t usLen_, uint32_t *pulMask_);
static void VcpDebug_SetLevel(VcpDebugLevelTypeDef eLevel_);
static void VcpDebug_SetTargetMask(uint32_t ulTargetMask_);
static void VcpDebug_TriggerHardFault(void) __attribute__((noreturn));

/*=============================================================================
 * Public Function Definitions
 *============================================================================*/
/**
 * @brief Initialize VCP debug filters.
 */
void VcpDebug_Init(void)
{
  (void)VcpPort_Init();

  ulTheDebugLevel = LEVEL_TRACE;
  ulTheDebugTargetMask = VCP_DBG_MASK_ALL;
}

/**
 * @brief Filtered printf-style debug output over the VCP DMA port.
 *
 * @details
 * Prints only when eLevel_ is at or below the configured debug level and
 * ulTargetMask_ intersects the configured target mask.
 *
 * @param[in] eLevel_       Message debug level.
 * @param[in] ulTargetMask_ Message target mask.
 * @param[in] pcFormat_     printf-style format string.
 */
void VcpDebugPrintf(VcpDebugLevelTypeDef eLevel_, uint32_t ulTargetMask_, const char *pcFormat_, ...)
{
  // Skip messages outside the active level or target filters.
  if (IsVcpDebugPrintfEnabled(eLevel_, ulTargetMask_) == 0U)
    return;

  // Reject invalid format strings.
  if (pcFormat_ == NULL)
  {
    VcpDebug_WriteString("[VCP_ERROR] printf null format\r\n");
    return;
  }

  char acPrintBuf[VCP_DEBUG_PRINTF_BUF_SIZE];
  va_list stArgs;
  int32_t lPrefixLen = 0;
  int32_t lLen = 0;
  uint16_t usLen = 0U;
  uint16_t usWritten = 0U;

  lPrefixLen = snprintf(
      acPrintBuf,
      sizeof(acPrintBuf),
      "[%s] [%s] ",
      GetVcpDebugLevelString(eLevel_),
      GetVcpDebugMaskString(ulTargetMask_));

  // Reject prefix formatting errors.
  if (lPrefixLen < 0)
  {
    VcpDebug_WriteString("[ERROR] [VCP] printf prefix error\r\n");
    return;
  }

  // Report local print buffer overrun instead of sending a truncated message.
  if (lPrefixLen >= (int32_t)sizeof(acPrintBuf))
  {
    VcpDebug_WriteString("[ERROR] [VCP] printf buffer overrun\r\n");
    return;
  }

  va_start(stArgs, pcFormat_);
  lLen = vsnprintf(
      &acPrintBuf[lPrefixLen],
      sizeof(acPrintBuf) - (uint32_t)lPrefixLen,
      pcFormat_,
      stArgs);
  va_end(stArgs);

  // Reject formatting errors.
  if (lLen < 0)
  {
    VcpDebug_WriteString("[ERROR] [VCP] printf format error\r\n");
    return;
  }

  // Report local print buffer overrun instead of sending a truncated message.
  if ((lPrefixLen + lLen) >= (int32_t)sizeof(acPrintBuf))
  {
    VcpDebug_WriteString("[ERROR] [VCP] printf buffer overrun\r\n");
    return;
  }

  usLen = (uint16_t)(lPrefixLen + lLen);
  usWritten = VcpDebug_Write((const uint8_t*)acPrintBuf, usLen);

  // Report TX ring overflow when the full formatted message could not queue.
  if (usWritten < usLen)
    VcpDebug_WriteString("[ERROR] [VCP] tx buffer overrun\r\n");
}

/**
 * @brief Process one newline-terminated VCP debug command.
 *
 * @param[in,out] pcCommand_ Command buffer, modified during parsing.
 */
bool HandleDebugVcpCommand(const VcpCommandTypeDef *pstCmd_)
{
  uint8_t ucIndex = 0U;
  VcpDebugLevelTypeDef eLevel = LEVEL_ERROR;
  uint32_t ulMask = 0U;
  bool bHandled = false;

  if ((pstCmd_ == NULL) || (pstCmd_->pcAction == NULL))
    return false;

  // Handle "debug.set" commands
  if (IsVcpTokenEqual(pstCmd_->pcAction, "set") != false)
  {
    for (ucIndex = 0U; ucIndex < pstCmd_->ucArgc; ucIndex++)
    {
      // Handle "debug.set level=<level>" argument
      if (IsVcpArgKeyEqual(pstCmd_->pacArgv[ucIndex], "level") != false)
      {
        if (ParseVcpDebugLevel(&pstCmd_->pacArgv[ucIndex][strlen("level=")], &eLevel) == false)
        {
          DPRINTF_VCP("ERR bad level\r\n");
          return true;
        }

        VcpDebug_SetLevel(eLevel);
        bHandled = true;
      }
      // Handle "debug.set mask=<mask list>" argument
      else if (IsVcpArgKeyEqual(pstCmd_->pacArgv[ucIndex], "mask") != false)
      {
        if (ParseVcpDebugTargetMaskList(&pstCmd_->pacArgv[ucIndex][strlen("mask=")], &ulMask) == false)
        {
          DPRINTF_VCP("ERR bad mask\r\n");
          return true;
        }

        VcpDebug_SetTargetMask(ulMask);
        bHandled = true;
      }
      else
      {
        return false;
      }
    }

    if (bHandled != false)
      DPRINTF_VCP("OK debug set\r\n");

    return bHandled;
  }

  // Handle "debug.get" commands
  if (IsVcpTokenEqual(pstCmd_->pcAction, "get") != false)
  {
    for (ucIndex = 0U; ucIndex < pstCmd_->ucArgc; ucIndex++)
    {
      // Handle "debug.get level"
      if (IsVcpTokenEqual(pstCmd_->pacArgv[ucIndex], "level") != false)
      {
        DPRINTF_VCP("level=%s\r\n", GetVcpDebugLevelString(ulTheDebugLevel));
        bHandled = true;
      }
      // Handle "debug.get mask"
      else if (IsVcpTokenEqual(pstCmd_->pacArgv[ucIndex], "mask") != false)
      {
        DPRINTF_VCP("mask=%s\r\n", GetVcpDebugMaskString(ulTheDebugTargetMask));
        bHandled = true;
      }
      else
      {
        return false;
      }

    }

    return bHandled;
  }

  // Handle "debug.hardfault" command
  if (IsVcpTokenEqual(pstCmd_->pcAction, "hardfault") != false)
  {
    DPRINTF_VCP("OK triggering hardfault\r\n");
    VcpDebug_TriggerHardFault();
  }

  // Handle "debug.help" command
  if (IsVcpTokenEqual(pstCmd_->pcAction, "help") != false)
  {
    DPRINTF_VCP("Available commands:\r\n");
    DPRINTF_VCP("  debug.set level=<level> mask=<mask>\r\n");
    DPRINTF_VCP("    <level>: error, warn, info, debug, trace\r\n");
    DPRINTF_VCP("    <mask>: comma-separated list of system,comm,thermal,feeding\r\n");
    DPRINTF_VCP("  debug.get level mask\r\n");
    DPRINTF_VCP("  debug.hardfault\r\n");
    DPRINTF_VCP("  debug.help\r\n");
    bHandled = true;

    return bHandled;
  }

  return false;
}

/*=============================================================================
 * Private Function Definitions
 *============================================================================*/
/**
 * @brief Queue bytes for DMA transmission on the ST-LINK Virtual COM Port.
 *
 * @details
 * Copies data into a circular TX buffer and starts DMA if the UART is idle.
 * The return value may be smaller than usLen_ if the circular buffer is full.
 *
 * @param[in] paucData_ Pointer to bytes to queue.
 * @param[in] usLen_    Number of bytes to queue.
 *
 * @return Number of bytes queued.
 */
static uint16_t VcpDebug_Write(const uint8_t *paucData_, uint16_t usLen_)
{
  return VcpPort_Write(paucData_, usLen_);
}

/**
 * @brief Queue a null-terminated string for DMA transmission.
 *
 * @param[in] pcString_ String to queue.
 *
 * @return Number of bytes queued.
 */
static uint16_t VcpDebug_WriteString(const char *pcString_)
{
  return VcpPort_WriteString(pcString_);
}

/**
 * @brief Intentionally escalate a UsageFault into HardFault for test coverage.
 */
static void VcpDebug_TriggerHardFault(void)
{
  SCB->SHCSR &= ~(SCB_SHCSR_MEMFAULTENA_Msk |
                  SCB_SHCSR_BUSFAULTENA_Msk |
                  SCB_SHCSR_USGFAULTENA_Msk);
  __DSB();
  __ISB();

  // Trigger a undefined instruciton hardfault
  __asm volatile ("udf #0");

  while (1)
  {
  }
}

/**
 * @brief Set the maximum debug level printed by VcpDebugPrintf().
 *
 * @param[in] eLevel_ Maximum enabled debug level.
 */
static void VcpDebug_SetLevel(VcpDebugLevelTypeDef eLevel_)
{
  if (eLevel_ <= LEVEL_TRACE)
    ulTheDebugLevel = eLevel_;
}

/**
 * @brief Set the enabled debug target mask for VcpDebugPrintf().
 *
 * @param[in] ulTargetMask_ Enabled target mask.
 */
static void VcpDebug_SetTargetMask(uint32_t ulTargetMask_)
{
  ulTheDebugTargetMask = ulTargetMask_;
}

/**
 * @brief Parse a debug level name.
 *
 * @param[in]  pcText_  Level text.
 * @param[out] peLevel_ Parsed level.
 *
 * @return true on success, false otherwise.
 */
static bool ParseVcpDebugLevel(const char *pcText_, VcpDebugLevelTypeDef *peLevel_)
{
  if ((pcText_ == NULL) || (peLevel_ == NULL))
    return false;

  if (IsVcpTokenEqual(pcText_, "error") != false)
    *peLevel_ = LEVEL_ERROR;
  else if (IsVcpTokenEqual(pcText_, "warn") != false)
    *peLevel_ = LEVEL_WARN;
  else if (IsVcpTokenEqual(pcText_, "info") != false)
    *peLevel_ = LEVEL_INFO;
  else if (IsVcpTokenEqual(pcText_, "debug") != false)
    *peLevel_ = LEVEL_DEBUG;
  else if (IsVcpTokenEqual(pcText_, "trace") != false)
    *peLevel_ = LEVEL_TRACE;
  else
    return false;

  return true;
}

/**
 * @brief Parse a comma-separated target mask list.
 *
 * @param[in]  pcText_  Comma-separated mask names.
 * @param[out] pulMask_ Parsed mask.
 *
 * @return true on success, false otherwise.
 */
static bool ParseVcpDebugTargetMaskList(const char *pcText_, uint32_t *pulMask_)
{
  const char *pcTokenStart = pcText_;
  const char *pcCursor = pcText_;
  uint32_t ulMask = 0U;
  uint32_t ulTokenMask = 0U;

  if ((pcText_ == NULL) || (pulMask_ == NULL) || (*pcText_ == '\0'))
    return false;

  while (true)
  {
    if ((*pcCursor == ',') || (*pcCursor == '\0') || (*pcCursor == '\r'))
    {
      if (ParseVcpDebugTargetMaskName(pcTokenStart, (uint16_t)(pcCursor - pcTokenStart), &ulTokenMask) == false)
        return false;

      ulMask |= ulTokenMask;

      if ((*pcCursor == '\0') || (*pcCursor == '\r'))
        break;

      pcTokenStart = pcCursor + 1;
    }

    pcCursor++;
  }

  *pulMask_ = ulMask;
  return true;
}

/**
 * @brief Parse one target mask name.
 *
 * @param[in]  pcText_  Target name text.
 * @param[in]  usLen_   Target name length.
 * @param[out] pulMask_ Parsed mask.
 *
 * @return true on success, false otherwise.
 */
static bool ParseVcpDebugTargetMaskName(const char *pcText_, uint16_t usLen_, uint32_t *pulMask_)
{
  if ((pcText_ == NULL) || (pulMask_ == NULL))
    return false;

  while ((usLen_ > 0U) && ((*pcText_ == ' ') || (*pcText_ == '\t')))
  {
    pcText_++;
    usLen_--;
  }

  while ((usLen_ > 0U) && ((pcText_[usLen_ - 1U] == ' ') || (pcText_[usLen_ - 1U] == '\t')))
    usLen_--;

  if (usLen_ == 0U)
    return false;

  if ((usLen_ == strlen("system")) && (strncmp(pcText_, "system", usLen_) == 0))
    *pulMask_ = DBG_MASK_SYSTEM;
  else if ((usLen_ == strlen("comm")) && (strncmp(pcText_, "comm", usLen_) == 0))
    *pulMask_ = DBG_MASK_COMM;
  else if ((usLen_ == strlen("thermal")) && (strncmp(pcText_, "thermal", usLen_) == 0))
    *pulMask_ = DBG_MASK_THERMAL;
  else if ((usLen_ == strlen("feeding")) && (strncmp(pcText_, "feeding", usLen_) == 0))
    *pulMask_ = DBG_MASK_FEEDING;
  else if ((usLen_ == strlen("all")) && (strncmp(pcText_, "all", usLen_) == 0))
    *pulMask_ = VCP_DBG_MASK_ALL;
  else if ((usLen_ == strlen("none")) && (strncmp(pcText_, "none", usLen_) == 0))
    *pulMask_ = DBG_MASK_NONE;
  else
    return false;

  return true;
}

/**
 * @brief Check whether a formatted debug message passes the filters.
 *
 * @param[in] eLevel_       Message debug level.
 * @param[in] ulTargetMask_ Message target mask.
 *
 * @return true when enabled, false when filtered.
 */
static bool IsVcpDebugPrintfEnabled(VcpDebugLevelTypeDef eLevel_, uint32_t ulTargetMask_)
{
  // VCP command responses always bypass the debug filters.
  if ((ulTargetMask_ & DBG_MASK_VCP) != 0U)
    return true;

  // Reject invalid debug levels.
  if (eLevel_ > LEVEL_TRACE)
    return false;

  // Higher numeric levels are more verbose.
  if (eLevel_ > ulTheDebugLevel)
    return false;

  // Target must intersect the enabled target mask.
  if ((ulTargetMask_ & ulTheDebugTargetMask) == 0U)
    return false;

  return true;
}

/**
 * @brief Convert a debug level to its print prefix string.
 *
 * @param[in] eLevel_ Debug level.
 *
 * @return Debug level string.
 */
static const char *GetVcpDebugLevelString(VcpDebugLevelTypeDef eLevel_)
{
  switch (eLevel_)
  {
    case LEVEL_ERROR:
      return "ERROR";

    case LEVEL_WARN:
      return "WARN ";

    case LEVEL_INFO:
      return "INFO ";

    case LEVEL_DEBUG:
      return "DEBUG";

    case LEVEL_TRACE:
      return "TRACE";

    default:
      return "UNKNOWN";
  }
}

/**
 * @brief Convert a debug target mask to its print prefix string.
 *
 * @param[in] ulMask_ Debug target mask.
 *
 * @return Debug target mask string.
 */
static const char* GetVcpDebugMaskString(uint32_t ulMask_)
{
  switch (ulMask_)
  {
    case DBG_MASK_NONE:
      return "NONE   ";

    case DBG_MASK_SYSTEM:
      return "SYSTEM ";

    case DBG_MASK_COMM:
      return "COMM   ";

    case DBG_MASK_THERMAL:
      return "THERMAL";

    case DBG_MASK_FEEDING:
      return "FEEDING";

    case VCP_DBG_MASK_ALL:
      return "ALL    ";

    case DBG_MASK_VCP:
      return "VCP    ";

    default:
      return "UNKNOWN";
  }
}
