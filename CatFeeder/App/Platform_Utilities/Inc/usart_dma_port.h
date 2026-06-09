/******************************************************************************
 * @file    usart_dma_port.h
 * @brief
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

#ifndef PLATFORM_UTILITIES_INC_USART_DMA_PORT_H_
#define PLATFORM_UTILITIES_INC_USART_DMA_PORT_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*=============================================================================
 * Includes
 *============================================================================*/
#include "main.h"
#include <stdint.h>

/*=============================================================================
* Public Macros
*============================================================================*/

/*=============================================================================
* Public Type Definitions
*============================================================================*/
typedef struct
{
  UART_HandleTypeDef *pstHuart;      /**< Associated UART handle */
  DMA_HandleTypeDef *pstHdmaRx;      /**< UART RX DMA handle */
  DMA_HandleTypeDef *pstHdmaTx;      /**< UART TX DMA handle */

  uint8_t *paucRxBuf;                /**< DMA RX buffer */
  uint16_t usRxSize;                 /**< RX buffer size */

  uint8_t *paucTxBuf;                /**< TX ring buffer */
  uint16_t usTxSize;                 /**< TX buffer size */

  volatile uint16_t usTxHead;        /**< TX write index */
  volatile uint16_t usTxTail;        /**< TX read index */

  volatile uint8_t ucTxDmaActive;    /**< TX DMA transfer active */
  uint16_t txDmaLen;                 /**< Current DMA TX length */
} UartDmaPortTypedef;

/*=============================================================================
* Public Constants
*============================================================================*/

/*=============================================================================
* Public Function Prototypes
*============================================================================*/
HAL_StatusTypeDef UartDmaPort_Init(UartDmaPortTypedef *pstPort_);
uint16_t UartDmaPort_Write(UartDmaPortTypedef *pstPort_, const uint8_t *paucData_, uint16_t usLen_);
uint16_t UartDmaPort_WriteString(UartDmaPortTypedef *pstPort_, const char *pcString_);
void UartDmaPort_TxCpltCallback(UartDmaPortTypedef *pstPort_, UART_HandleTypeDef *pstUart_);
void UartDmaPort_RxEventCallback(UartDmaPortTypedef *pstPort_, UART_HandleTypeDef *pstUart_, uint16_t usSize_);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_UTILITIES_INC_USART_DMA_PORT_H_ */
