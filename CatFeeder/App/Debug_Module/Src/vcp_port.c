/******************************************************************************
 * @file    vcp_port.c
 * @brief   VCP transport interface over the UART DMA port.
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
#include "vcp_port.h"
#include "usart.h"
#include "usart_dma_port.h"
#include <string.h>

/*=============================================================================
 * Private Macros
 *============================================================================*/
#define VCP_PORT_TX_BUF_SIZE  (512U)
#define VCP_PORT_RX_BUF_SIZE  (512U)

/*=============================================================================
 * Private Type Definitions
 *============================================================================*/
typedef struct
{
  uint8_t aucTxBuf[VCP_PORT_TX_BUF_SIZE];
  uint8_t aucRxBuf[VCP_PORT_RX_BUF_SIZE];
} VcpPortBufferTypeDef;

/*=============================================================================
 * Private Variables
 *============================================================================*/
static VcpPortBufferTypeDef stTheVcpPortBuffers;
static UartDmaPortTypeDef stTheVcpPort;

/*=============================================================================
 * Private Function Prototypes
 *============================================================================*/
static void ConfigureVcpPort(void);

/*=============================================================================
 * Public Function Definitions
 *============================================================================*/
/**
 * @brief Initialize the VCP UART DMA transport.
 *
 * @return true on success, false otherwise.
 */
bool VcpPort_Init(void)
{
  ConfigureVcpPort();

  (void)memset(stTheVcpPortBuffers.aucRxBuf, 0, sizeof(stTheVcpPortBuffers.aucRxBuf));

  return (UartDmaPort_Init(&stTheVcpPort) == HAL_OK);
}

/**
 * @brief Wait for a VCP RX idle-line event from the UART DMA port.
 *
 * @param[in] ulTimeout_ Timeout in RTOS ticks.
 *
 * @return true when RX data is available, false on timeout or error.
 */
bool VcpPort_WaitForRxEvent(uint32_t ulTimeout_)
{
  uint32_t ulFlags = 0U;

  // Reject waits before the DMA port creates its event flag object.
  if (stTheVcpPort.hEventFlags == NULL)
    return false;

  ulFlags = osEventFlagsWait(
      stTheVcpPort.hEventFlags,
      UART_DMA_PORT_RX_EVENT_FLAG,
      osFlagsWaitAny,
      ulTimeout_);

  // Ignore timeout and RTOS error return codes.
  if ((ulFlags & osFlagsError) != 0U)
    return false;

  // Wake only when the RX idle-line flag was actually observed.
  if ((ulFlags & UART_DMA_PORT_RX_EVENT_FLAG) == 0U)
    return false;

  return true;
}

/**
 * @brief Read bytes from the VCP RX ring.
 *
 * @param[out] paucData_ Destination buffer.
 * @param[in]  usLen_    Maximum number of bytes to read.
 *
 * @return Number of bytes read.
 */
uint16_t VcpPort_Read(uint8_t *paucData_, uint16_t usLen_)
{
  return UartDmaPort_Read(&stTheVcpPort, paucData_, usLen_);
}

/**
 * @brief Write bytes to the VCP TX ring.
 *
 * @param[in] paucData_ Bytes to queue.
 * @param[in] usLen_    Number of bytes to queue.
 *
 * @return Number of bytes queued.
 */
uint16_t VcpPort_Write(const uint8_t *paucData_, uint16_t usLen_)
{
  return UartDmaPort_Write(&stTheVcpPort, paucData_, usLen_);
}

/**
 * @brief Write a string to the VCP TX ring.
 *
 * @param[in] pcString_ String to queue.
 *
 * @return Number of bytes queued.
 */
uint16_t VcpPort_WriteString(const char *pcString_)
{
  return UartDmaPort_WriteString(&stTheVcpPort, pcString_);
}

/*=============================================================================
 * Private Function Definitions
 *============================================================================*/
/**
 * @brief Configure the reusable UART DMA port for the ST-LINK VCP.
 */
static void ConfigureVcpPort(void)
{
  stTheVcpPort.pstHuart = &hlpuart1;
  stTheVcpPort.pstHdmaRx = &hdma_lpuart1_rx;
  stTheVcpPort.pstHdmaTx = &hdma_lpuart1_tx;
  stTheVcpPort.paucRxBuf = stTheVcpPortBuffers.aucRxBuf;
  stTheVcpPort.usRxSize = sizeof(stTheVcpPortBuffers.aucRxBuf);
  stTheVcpPort.paucTxBuf = stTheVcpPortBuffers.aucTxBuf;
  stTheVcpPort.usTxSize = sizeof(stTheVcpPortBuffers.aucTxBuf);
}
