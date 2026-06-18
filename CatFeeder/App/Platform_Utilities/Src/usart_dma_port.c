/******************************************************************************
 * @file    usart_dma_port.c
 * @brief   UART DMA port implementation.
 *
 * @project PawPlate - Intelligent Wet Cat Food Dispensing System
 * @course  ECE 498 Engineering Design Project
 * @team    Team 53
 * @authors
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
#include "usart_dma_port.h"

/*=============================================================================
 * Private Macros
 *============================================================================*/
#define UART_DMA_PORT_MAX_PORTS      (4U)
#define IS_SIZE_POWER_OF_TWO(size_)  (((size_) != 0U) && (((size_) & ((size_) - 1U)) == 0U))

/*=============================================================================
 * Private Type Definitions
 *============================================================================*/

/*=============================================================================
 * Private Variables
 *============================================================================*/
static UartDmaPortTypeDef *pastTheUartDmaPorts[UART_DMA_PORT_MAX_PORTS];

/*=============================================================================
 * Private Function Prototypes
 *============================================================================*/
static HAL_StatusTypeDef StartUartDmaPortTransfer(UartDmaPortTypeDef *pstPort_, uint8_t *paucData_, uint16_t usLen_);
static HAL_StatusTypeDef EnableUartDmaPortReceive(UartDmaPortTypeDef *pstPort_);
static void TryStartUartDmaPortTx(UartDmaPortTypeDef *pstPort_);
static uint16_t GetUartDmaPortTxFreeSpace(UartDmaPortTypeDef *pstPort_);
static uint16_t AdvanceUartDmaPortIndex(uint16_t usRingSize_, uint16_t usIndex_, uint16_t usCount_);
static uint32_t EnterUartDmaPortCritical(void);
static void ExitUartDmaPortCritical(uint32_t ulPrimask_);
static bool IsUartDmaPortValid(UartDmaPortTypeDef *pstPort_);
static HAL_StatusTypeDef RegisterUartDmaPort(UartDmaPortTypeDef *pstPort_);
static HAL_StatusTypeDef RegisterUartDmaPortCallbacks(UartDmaPortTypeDef *pstPort_);
static UartDmaPortTypeDef *FindUartDmaPort(UART_HandleTypeDef *pstUart_);
static void UartDmaPortTxCpltCallback(UART_HandleTypeDef *pstUart_);
static void UartDmaPortRxEventCallback(UART_HandleTypeDef *pstUart_, uint16_t usSize_);

/*=============================================================================
 * Public Function Definitions
 *============================================================================*/
/**
 * @brief Initialize a UART DMA port and enable receive-to-idle DMA.
 *
 * @param[in,out] pstPort_ UART DMA port object.
 *
 * @return HAL status.
 */
HAL_StatusTypeDef UartDmaPort_Init(UartDmaPortTypeDef *pstPort_)
{
  // Reject incomplete port objects.
  if (IsUartDmaPortValid(pstPort_) == false)
    return HAL_ERROR;

  pstPort_->usTxHead = 0U;
  pstPort_->usTxTail = 0U;
  pstPort_->usRxHead = 0U;
  pstPort_->usRxTail = 0U;
  pstPort_->txDmaLen = 0U;
  pstPort_->bTxDmaActive = false;
  pstPort_->hEventFlags = osEventFlagsNew(NULL);
  if (pstPort_->hEventFlags == NULL)
    return HAL_ERROR;

  if (RegisterUartDmaPort(pstPort_) != HAL_OK)
    return HAL_ERROR;

  if (RegisterUartDmaPortCallbacks(pstPort_) != HAL_OK)
    return HAL_ERROR;

  return EnableUartDmaPortReceive(pstPort_);
}

/**
 * @brief Read bytes received by RX DMA from the RX ring buffer.
 *
 * @param[in,out] pstPort_  UART DMA port object.
 * @param[out]    paucData_ Destination buffer.
 * @param[in]     usLen_    Maximum number of bytes to read.
 *
 * @return Number of bytes read.
 */
uint16_t UartDmaPort_Read(UartDmaPortTypeDef *pstPort_, uint8_t *paucData_, uint16_t usLen_)
{
  uint16_t usRead = 0U;
  uint32_t ulPrimask = 0U;

  // Reject empty or invalid reads.
  if ((IsUartDmaPortValid(pstPort_) == false) || (paucData_ == NULL) || (usLen_ == 0U))
    return 0U;

  ulPrimask = EnterUartDmaPortCritical();

  // Copy until the request is filled or the RX ring is empty.
  while ((usRead < usLen_) && (pstPort_->usRxTail != pstPort_->usRxHead))
  {
    paucData_[usRead] = pstPort_->paucRxBuf[pstPort_->usRxTail];
    pstPort_->usRxTail = AdvanceUartDmaPortIndex(pstPort_->usRxSize, pstPort_->usRxTail, 1U);
    usRead++;
  }

  ExitUartDmaPortCritical(ulPrimask);

  return usRead;
}

/**
 * @brief Queue bytes for DMA transmission.
 *
 * @param[in,out] pstPort_  UART DMA port object.
 * @param[in]     paucData_ Bytes to queue.
 * @param[in]     usLen_    Number of bytes to queue.
 *
 * @return Number of bytes queued.
 */
uint16_t UartDmaPort_Write(UartDmaPortTypeDef *pstPort_, const uint8_t *paucData_, uint16_t usLen_)
{
  uint16_t usWritten = 0U;
  uint32_t ulPrimask = 0U;

  // Reject empty or invalid writes.
  if ((IsUartDmaPortValid(pstPort_) == false) || (paucData_ == NULL) || (usLen_ == 0U))
    return 0U;

  ulPrimask = EnterUartDmaPortCritical();

  // Copy until the request is queued or the ring is full.
  while ((usWritten < usLen_) && (GetUartDmaPortTxFreeSpace(pstPort_) > 0U))
  {
    pstPort_->paucTxBuf[pstPort_->usTxHead] = paucData_[usWritten];
    pstPort_->usTxHead = AdvanceUartDmaPortIndex(pstPort_->usTxSize, pstPort_->usTxHead, 1U);
    usWritten++;
  }

  ExitUartDmaPortCritical(ulPrimask);

  TryStartUartDmaPortTx(pstPort_);

  return usWritten;
}

/**
 * @brief Queue a null-terminated string for DMA transmission.
 *
 * @param[in,out] pstPort_  UART DMA port object.
 * @param[in]     pcString_ String to queue.
 *
 * @return Number of bytes queued.
 */
uint16_t UartDmaPort_WriteString(UartDmaPortTypeDef *pstPort_, const char *pcString_)
{
  uint16_t usLen = 0U;

  // Reject invalid string pointers.
  if (pcString_ == NULL)
    return 0U;

  // Count the string length without exceeding the uint16_t API limit.
  while ((usLen < UINT16_MAX) && (pcString_[usLen] != '\0'))
    usLen++;

  return UartDmaPort_Write(pstPort_, (const uint8_t *)pcString_, usLen);
}

/*=============================================================================
 * Private Function Definitions
 *============================================================================*/
/**
 * @brief Registered UART TX complete callback dispatcher.
 *
 * @param[in] pstUart_ UART that completed transmission.
 */
static void UartDmaPortTxCpltCallback(UART_HandleTypeDef *pstUart_)
{
  UartDmaPortTypeDef *pstPort = FindUartDmaPort(pstUart_);

  // Ignore callbacks from UARTs that do not use this driver.
  if (pstPort == NULL)
    return;

  pstPort->usTxTail = AdvanceUartDmaPortIndex(pstPort->usTxSize, pstPort->usTxTail, pstPort->txDmaLen);
  pstPort->txDmaLen = 0U;
  pstPort->bTxDmaActive = false;

  // Try to start a new transfer if messages exist in queue.
  TryStartUartDmaPortTx(pstPort);
}

/**
 * @brief Registered UART receive-to-idle callback dispatcher.
 *
 * @param[in] pstUart_ UART that received data.
 * @param[in] usSize_  Number of bytes received.
 */
static void UartDmaPortRxEventCallback(UART_HandleTypeDef *pstUart_, uint16_t usSize_)
{
  UartDmaPortTypeDef *pstPort = FindUartDmaPort(pstUart_);

  // Ignore callbacks from UARTs that do not use this driver.
  if (pstPort == NULL)
    return;

  //Update the head index based on the number of bytes received.
  pstPort->usRxHead = AdvanceUartDmaPortIndex(pstPort->usRxSize, 0U, usSize_);

  (void)osEventFlagsSet(pstPort->hEventFlags, UART_DMA_PORT_RX_EVENT_FLAG);
}

/**
 * @brief Start receive-to-idle DMA on a UART DMA port.
 *
 * @param[in,out] pstPort_ UART DMA port object.
 *
 * @return HAL status.
 */
static HAL_StatusTypeDef EnableUartDmaPortReceive(UartDmaPortTypeDef *pstPort_)
{
  HAL_StatusTypeDef eStatus = HAL_ERROR;

  // Reject incomplete receive configuration.
  if (IsUartDmaPortValid(pstPort_) == false)
    return HAL_ERROR;

  eStatus = HAL_UARTEx_ReceiveToIdle_DMA(
      pstPort_->pstHuart,
      pstPort_->paucRxBuf,
      pstPort_->usRxSize);

  // Disable half transfer callback.
  if (pstPort_->pstHdmaRx != NULL)
    __HAL_DMA_DISABLE_IT(pstPort_->pstHdmaRx, DMA_IT_HT);

  return eStatus;
}

/**
 * @brief Attempt to start a DMA TX using the next contiguous ring segment.
 *
 * @param[in,out] pstPort_ UART DMA port object.
 */
static void TryStartUartDmaPortTx(UartDmaPortTypeDef *pstPort_)
{
  uint16_t usLen = 0U;
  uint16_t usTail = 0U;
  uint32_t ulPrimask = 0U;

  // Reject incomplete port objects.
  if (IsUartDmaPortValid(pstPort_) == false)
    return;

  ulPrimask = EnterUartDmaPortCritical();

  // Nothing to start if DMA is busy or the ring is empty.
  if ((pstPort_->bTxDmaActive != false) || (pstPort_->usTxHead == pstPort_->usTxTail))
  {
    ExitUartDmaPortCritical(ulPrimask);
    return;
  }

  usTail = pstPort_->usTxTail;

  // Send only the next contiguous ring segment.
  if (pstPort_->usTxHead > usTail)
    usLen = pstPort_->usTxHead - usTail;
  else
    usLen = pstPort_->usTxSize - usTail;

  pstPort_->txDmaLen = usLen;
  pstPort_->bTxDmaActive = true;

  ExitUartDmaPortCritical(ulPrimask);

  // If HAL cannot start DMA now, release the active flag for retry later.
  if (StartUartDmaPortTransfer(pstPort_, &pstPort_->paucTxBuf[usTail], usLen) != HAL_OK)
  {
    ulPrimask = EnterUartDmaPortCritical();
    pstPort_->txDmaLen = 0U;
    pstPort_->bTxDmaActive = false;
    ExitUartDmaPortCritical(ulPrimask);
  }
}

/**
 * @brief Get available bytes in the TX ring.
 *
 * @param[in] pstPort_ UART DMA port object.
 *
 * @return Number of free bytes available for new data.
 */
static uint16_t GetUartDmaPortTxFreeSpace(UartDmaPortTypeDef *pstPort_)
{
  uint16_t usHead = pstPort_->usTxHead;
  uint16_t usTail = pstPort_->usTxTail;

  // Head after tail means used bytes do not wrap.
  if (usHead >= usTail)
    return (pstPort_->usTxSize - (usHead - usTail) - 1U);

  return (usTail - usHead - 1U);
}

/**
 * @brief Advance a ring index by count bytes.
 *
 * @param[in] usRingSize_ Number of entries in the ring.
 * @param[in] usIndex_    Current ring index.
 * @param[in] usCount_    Number of bytes to advance.
 *
 * @return Advanced ring index.
 */
static uint16_t AdvanceUartDmaPortIndex(
  uint16_t usRingSize_,
  uint16_t usIndex_,
  uint16_t usCount_)
{
  if (usRingSize_ == 0U)
    return 0U;

  return (uint16_t)(((uint32_t)usIndex_ + usCount_) & (usRingSize_ - 1U));
}

/**
 * @brief Enter a short critical section and capture prior IRQ state.
 *
 * @return Previous PRIMASK value.
 */
static uint32_t EnterUartDmaPortCritical(void)
{
  uint32_t ulPrimask = __get_PRIMASK();
  __disable_irq();
  return ulPrimask;
}

/**
 * @brief Leave a critical section without enabling IRQs unexpectedly.
 *
 * @param[in] ulPrimask_ PRIMASK value returned by EnterUartDmaPortCritical().
 */
static void ExitUartDmaPortCritical(uint32_t ulPrimask_)
{
  // Re-enable IRQs only if this code disabled them.
  if (ulPrimask_ == 0U)
    __enable_irq();
}

/**
 * @brief Start a DMA transmission on a UART DMA port.
 *
 * @param[in,out] pstPort_  UART DMA port object.
 * @param[in]     paucData_ Pointer to the transmit data buffer.
 * @param[in]     usLen_    Number of bytes to transmit.
 *
 * @return HAL status.
 */
static HAL_StatusTypeDef StartUartDmaPortTransfer(
  UartDmaPortTypeDef *pstPort_,
  uint8_t *paucData_,
  uint16_t usLen_)
{
  // Reject invalid DMA transfer requests.
  if ((IsUartDmaPortValid(pstPort_) == false) || (paucData_ == NULL) || (usLen_ == 0U))
    return HAL_ERROR;

  return HAL_UART_Transmit_DMA(pstPort_->pstHuart, paucData_, usLen_);
}

/**
 * @brief Check whether a UART DMA port object has the required fields.
 *
 * @param[in] pstPort_ UART DMA port object.
 *
 * @return true when valid, false otherwise.
 */
static bool IsUartDmaPortValid(UartDmaPortTypeDef *pstPort_)
{
  // Reject missing object pointers.
  if (pstPort_ == NULL)
    return false;

  // Reject incomplete UART/DMA buffer configuration.
  if ((pstPort_->pstHuart == NULL) ||
      (pstPort_->pstHdmaRx == NULL) ||
      (pstPort_->pstHdmaTx == NULL) ||
      (pstPort_->paucRxBuf == NULL) ||
      (pstPort_->paucTxBuf == NULL) ||
      (pstPort_->usRxSize == 0U) ||
      (pstPort_->usTxSize < 2U) ||
      (IS_SIZE_POWER_OF_TWO(pstPort_->usRxSize) == false) ||
      (IS_SIZE_POWER_OF_TWO(pstPort_->usTxSize) == false))
  {
    return false;
  }

  return true;
}

/**
 * @brief Register a UART DMA port for HAL callback dispatch.
 *
 * @param[in,out] pstPort_ UART DMA port object.
 *
 * @return HAL status.
 */
static HAL_StatusTypeDef RegisterUartDmaPort(UartDmaPortTypeDef *pstPort_)
{
  uint16_t usIndex = 0U;

  // Reject incomplete port objects.
  if (IsUartDmaPortValid(pstPort_) == false)
    return HAL_ERROR;

  for (usIndex = 0U; usIndex < UART_DMA_PORT_MAX_PORTS; usIndex++)
  {
    // Already registered.
    if (pastTheUartDmaPorts[usIndex] == pstPort_)
      return HAL_OK;
  }

  for (usIndex = 0U; usIndex < UART_DMA_PORT_MAX_PORTS; usIndex++)
  {
    // Use the first empty registry slot.
    if (pastTheUartDmaPorts[usIndex] == NULL)
    {
      pastTheUartDmaPorts[usIndex] = pstPort_;
      return HAL_OK;
    }
  }

  return HAL_ERROR;
}

/**
 * @brief Register HAL UART callbacks used by a UART DMA port.
 *
 * @param[in,out] pstPort_ UART DMA port object.
 *
 * @return HAL status.
 */
static HAL_StatusTypeDef RegisterUartDmaPortCallbacks(UartDmaPortTypeDef *pstPort_)
{
  // Reject incomplete port objects.
  if (IsUartDmaPortValid(pstPort_) == false)
    return HAL_ERROR;

  if (HAL_UART_RegisterCallback(pstPort_->pstHuart, HAL_UART_TX_COMPLETE_CB_ID, UartDmaPortTxCpltCallback) != HAL_OK)
    return HAL_ERROR;

  if (HAL_UART_RegisterRxEventCallback(pstPort_->pstHuart, UartDmaPortRxEventCallback) != HAL_OK)
    return HAL_ERROR;

  return HAL_OK;
}

/**
 * @brief Find the registered UART DMA port for a HAL UART handle.
 *
 * @param[in] pstUart_ HAL UART handle.
 *
 * @return Matching port object, or NULL.
 */
static UartDmaPortTypeDef *FindUartDmaPort(UART_HandleTypeDef *pstUart_)
{
  uint16_t usIndex = 0U;

  // Reject invalid UART handles.
  if (pstUart_ == NULL)
    return NULL;

  for (usIndex = 0U; usIndex < UART_DMA_PORT_MAX_PORTS; usIndex++)
  {
    // Match callbacks by HAL UART handle.
    if ((pastTheUartDmaPorts[usIndex] != NULL) && (pastTheUartDmaPorts[usIndex]->pstHuart == pstUart_))
      return pastTheUartDmaPorts[usIndex];
  }

  return NULL;
}
