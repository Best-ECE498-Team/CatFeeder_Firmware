/******************************************************************************
 * @file    uart_dma_port.h
 * @brief   UART DMA port interface.
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

#ifndef INC_UART_DMA_PORT_H_
#define INC_UART_DMA_PORT_H_

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * Includes
 *============================================================================*/
#include "stm32g4xx_hal.h"
#include <stdbool.h>
#include "cmsis_os.h"

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
  uint16_t usRxSize;                 /**< RX buffer size, must be power of 2 */

  uint8_t *paucTxBuf;                /**< TX ring buffer */
  uint16_t usTxSize;                 /**< TX buffer size, must be power of 2 */

  volatile uint16_t usTxHead;        /**< TX DMA write index */
  volatile uint16_t usTxTail;        /**< TX API read index */

  volatile uint16_t usRxHead;        /**< RX DMA write index */
  volatile uint16_t usRxTail;        /**< RX API read index */

  volatile bool bTxDmaActive;        /**< TX DMA transfer active */
  uint16_t txDmaLen;                 /**< Current DMA TX length */

  osEventFlagsId_t hEventFlags;      /**< Event flags for signaling between DMA ISR and API */
} UartDmaPortTypeDef;

/*=============================================================================
* Public Constants
*============================================================================*/
#define UART_DMA_PORT_TX_COMPLETE_FLAG (1U << 0)      /**< Event flag indicating DMA TX completion */
#define UART_DMA_PORT_RX_EVENT_FLAG    (1U << 1)      /**< Event flag indicating DMA RX event (idle line) */
#define UART_DMA_PORT_EVENT_FLAGS_MAX  (1U << 24 - 1) /**< Maximum event flags supported by FreeRTOS */

/*=============================================================================
* Public Function Prototypes
*============================================================================*/
HAL_StatusTypeDef UartDmaPort_Init(UartDmaPortTypeDef *pstPort_);
uint16_t UartDmaPort_Read(UartDmaPortTypeDef *pstPort_, uint8_t *paucData_, uint16_t usLen_);
uint16_t UartDmaPort_Write(UartDmaPortTypeDef *pstPort_, const uint8_t *paucData_, uint16_t usLen_);
uint16_t UartDmaPort_WriteString(UartDmaPortTypeDef *pstPort_, const char *pcString_);

#ifdef __cplusplus
}
#endif

#endif /* INC_UART_DMA_PORT_H_ */
