/******************************************************************************
 * @file    vcp_debug.c
 * @brief   This file provides code for the virtual com port debugging message
 *          transfer and command handling
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
#include "usart.h"
#include <stdarg.h>
#include <stdio.h>
#include "usart_dma_port.h"

/*=============================================================================
 * Private Macros
 *============================================================================*/
#define VCP_DEBUG_TX_BUF_SIZE     (512U)
#define VCP_DEBUG_RX_BUF_SIZE     (128U)
#define VCP_DEBUG_PRINTF_BUF_SIZE (32U)

/*=============================================================================
 * Private Type Definitions
 *============================================================================*/
typedef struct
{
  uint8_t aucTxBuf[VCP_DEBUG_TX_BUF_SIZE];
  uint8_t aucRxBuf[VCP_DEBUG_RX_BUF_SIZE];
} DebugBufferTypedef;

/*=============================================================================
 * Private Variables
 *============================================================================*/
static DebugBufferTypedef stTheDebugBuffers;
static UartDmaPortTypedef stTheVcpDebugPort;
static VcpDebugLevelTypedef ulTheDebugLevel;
static uint32_t ulTheDebugTargetMask;

/*=============================================================================
 * Private Function Prototypes
 *============================================================================*/
static void ConfigureVcpDebugPort(void);
static uint8_t IsVcpDebugPrintfEnabled(VcpDebugLevelTypedef eLevel_, uint32_t ulTargetMask_);
static const char* GetVcpDebugLevelString(VcpDebugLevelTypedef eLevel_);
static uint16_t VcpDebug_Write(const uint8_t* paucData_, uint16_t usLen_);
static uint16_t VcpDebug_WriteString(const char* pcString_);

/*=============================================================================
 * Public Function Definitions
 *============================================================================*/
/**
 * @brief Initialize debug buffer and enable dma receive
 */
void VcpDebug_Init(void)
{
  ConfigureVcpDebugPort();

  ulTheDebugLevel = LEVEL_TRACE;
  ulTheDebugTargetMask = VCP_DBG_MASK_ALL;

  (void)UartDmaPort_Init(&stTheVcpDebugPort);
}

/**
 * @brief Set the maximum debug level printed by VcpDebugPrintf().
 *
 * @param[in] eLevel_ Maximum enabled debug level.
 */
void VcpDebug_SetLevel(VcpDebugLevelTypedef eLevel_)
{
  if (eLevel_ <= LEVEL_TRACE)
    ulTheDebugLevel = eLevel_;
}

/**
 * @brief Set the enabled debug target mask for VcpDebugPrintf().
 *
 * @param[in] ulTargetMask_ Enabled target mask.
 */
void VcpDebug_SetTargetMask(uint32_t ulTargetMask_)
{
  ulTheDebugTargetMask = ulTargetMask_;
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
void VcpDebugPrintf(VcpDebugLevelTypedef eLevel_, uint32_t ulTargetMask_, const char* pcFormat_, ...)
{
  char acPrintBuf[VCP_DEBUG_PRINTF_BUF_SIZE];
  va_list stArgs;
  int32_t lPrefixLen = 0;
  int32_t lLen = 0;
  uint16_t usLen = 0U;
  uint16_t usWritten = 0U;

  // Reject invalid format strings.
  if (pcFormat_ == NULL)
  {
    VcpDebug_WriteString("[VCP_ERROR] printf null format\r\n");
    return;
  }

  // Skip messages outside the active level or target filters.
  if (IsVcpDebugPrintfEnabled(eLevel_, ulTargetMask_) == 0U)
    return;

  lPrefixLen = snprintf(
      acPrintBuf,
      sizeof(acPrintBuf),
      "[%s] [0x%lX] ",
      GetVcpDebugLevelString(eLevel_),
      (uint32_t)ulTargetMask_);

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

/*=============================================================================
 * Private Function Definitions
 *============================================================================*/
/**
 * @brief Configure the reusable UART DMA port for the ST-LINK VCP.
 */
static void ConfigureVcpDebugPort(void)
{
  stTheVcpDebugPort.pstHuart = &hlpuart1;
  stTheVcpDebugPort.pstHdmaRx = &hdma_lpuart1_rx;
  stTheVcpDebugPort.pstHdmaTx = &hdma_lpuart1_tx;
  stTheVcpDebugPort.paucRxBuf = stTheDebugBuffers.aucRxBuf;
  stTheVcpDebugPort.usRxSize = sizeof(stTheDebugBuffers.aucRxBuf);
  stTheVcpDebugPort.paucTxBuf = stTheDebugBuffers.aucTxBuf;
  stTheVcpDebugPort.usTxSize = sizeof(stTheDebugBuffers.aucTxBuf);
}

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
  return UartDmaPort_Write(&stTheVcpDebugPort, paucData_, usLen_);
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
  return UartDmaPort_WriteString(&stTheVcpDebugPort, pcString_);
}

/**
 * @brief Check whether a formatted debug message passes the filters.
 *
 * @param[in] eLevel_       Message debug level.
 * @param[in] ulTargetMask_ Message target mask.
 *
 * @return 1 when enabled, 0 when filtered.
 */
static uint8_t IsVcpDebugPrintfEnabled(VcpDebugLevelTypedef eLevel_, uint32_t ulTargetMask_)
{
  // Reject invalid debug levels.
  if (eLevel_ > LEVEL_TRACE)
    return 0U;

  // Higher numeric levels are more verbose.
  if (eLevel_ > ulTheDebugLevel)
    return 0U;

  // Target must intersect the enabled target mask.
  if ((ulTargetMask_ & ulTheDebugTargetMask) == 0U)
    return 0U;

  return 1U;
}

/**
 * @brief Convert a debug level to its print prefix string.
 *
 * @param[in] eLevel_ Debug level.
 *
 * @return Debug level string.
 */
static const char* GetVcpDebugLevelString(VcpDebugLevelTypedef eLevel_)
{
  switch (eLevel_)
  {
    case LEVEL_ERROR:
      return "ERROR";

    case LEVEL_WARN:
      return "WARN";

    case LEVEL_INFO:
      return "INFO";

    case LEVEL_DEBUG:
      return "DEBUG";

    case LEVEL_TRACE:
      return "TRACE";

    default:
      return "UNKNOWN";
  }
}
