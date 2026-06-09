/******************************************************************************
 * @file    usart_dma_port.c
 * @brief
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
#define UART_DMA_PORT_MAX_PORTS (4U)

/*=============================================================================
 * Private Type Definitions
 *============================================================================*/

/*=============================================================================
 * Private Variables
 *============================================================================*/
static UartDmaPortTypedef *apstTheUartDmaPorts[UART_DMA_PORT_MAX_PORTS];

/*=============================================================================
 * Private Function Prototypes
 *============================================================================*/
static HAL_StatusTypeDef StartUartDmaPortTransfer(UartDmaPortTypedef *pstPort_, uint8_t *paucData_, uint16_t usLen_);
static HAL_StatusTypeDef EnableUartDmaPortReceive(UartDmaPortTypedef *pstPort_);
static void TryStartUartDmaPortTx(UartDmaPortTypedef *pstPort_);
static uint16_t GetUartDmaPortTxFreeSpace(UartDmaPortTypedef *pstPort_);
static uint16_t AdvanceUartDmaPortIndex(UartDmaPortTypedef *pstPort_, uint16_t usIndex_, uint16_t usCount_);
static uint32_t EnterUartDmaPortCritical(void);
static void ExitUartDmaPortCritical(uint32_t ulPrimask_);
static uint8_t IsUartDmaPortValid(UartDmaPortTypedef *pstPort_);
static HAL_StatusTypeDef RegisterUartDmaPort(UartDmaPortTypedef *pstPort_);
static UartDmaPortTypedef *FindUartDmaPort(UART_HandleTypeDef *pstUart_);

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
HAL_StatusTypeDef UartDmaPort_Init(UartDmaPortTypedef *pstPort_)
{
  // Reject incomplete port objects.
  if (IsUartDmaPortValid(pstPort_) == 0U)
    return HAL_ERROR;

  pstPort_->usTxHead = 0U;
  pstPort_->usTxTail = 0U;
  pstPort_->txDmaLen = 0U;
  pstPort_->ucTxDmaActive = 0U;

  if (RegisterUartDmaPort(pstPort_) != HAL_OK)
    return HAL_ERROR;

  return EnableUartDmaPortReceive(pstPort_);
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
uint16_t UartDmaPort_Write(UartDmaPortTypedef *pstPort_, const uint8_t *paucData_, uint16_t usLen_)
{
  uint16_t usWritten = 0U;
  uint32_t ulPrimask = 0U;

  // Reject empty or invalid writes.
  if ((IsUartDmaPortValid(pstPort_) == 0U) || (paucData_ == NULL) || (usLen_ == 0U))
    return 0U;

  ulPrimask = EnterUartDmaPortCritical();

  // Copy until the request is queued or the ring is full.
  while ((usWritten < usLen_) && (GetUartDmaPortTxFreeSpace(pstPort_) > 0U))
  {
    pstPort_->paucTxBuf[pstPort_->usTxHead] = paucData_[usWritten];
    pstPort_->usTxHead = AdvanceUartDmaPortIndex(pstPort_, pstPort_->usTxHead, 1U);
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
uint16_t UartDmaPort_WriteString(UartDmaPortTypedef *pstPort_, const char *pcString_)
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

/**
 * @brief UART TX complete hook for a UART DMA port.
 *
 * @param[in,out] pstPort_ UART DMA port object.
 * @param[in]     pstUart_ UART that completed transmission.
 */
void UartDmaPort_TxCpltCallback(
    UartDmaPortTypedef *pstPort_,
    UART_HandleTypeDef *pstUart_)
{
  // Ignore callbacks from unrelated UARTs.
  if ((IsUartDmaPortValid(pstPort_) == 0U) || (pstUart_ != pstPort_->pstHuart))
    return;

  pstPort_->usTxTail = AdvanceUartDmaPortIndex(pstPort_, pstPort_->usTxTail, pstPort_->txDmaLen);
  pstPort_->txDmaLen = 0U;
  pstPort_->ucTxDmaActive = 0U;

  TryStartUartDmaPortTx(pstPort_);
}

/**
 * @brief UART receive-to-idle hook for a UART DMA port.
 *
 * @param[in,out] pstPort_ UART DMA port object.
 * @param[in]     pstUart_ UART that received data.
 * @param[in]     usSize_  Number of bytes received.
 */
void UartDmaPort_RxEventCallback(
    UartDmaPortTypedef *pstPort_,
    UART_HandleTypeDef *pstUart_,
    uint16_t usSize_)
{
  (void)usSize_;

  // Ignore callbacks from unrelated UARTs.
  if ((IsUartDmaPortValid(pstPort_) == 0U) || (pstUart_ != pstPort_->pstHuart))
    return;

  (void)EnableUartDmaPortReceive(pstPort_);
}

/**
 * @brief HAL receive-to-idle callback dispatcher for registered UART DMA ports.
 *
 * @param[in] huart UART that received data.
 * @param[in] Size  Number of bytes received.
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  UartDmaPortTypedef *pstPort = FindUartDmaPort(huart);

  // Ignore callbacks from UARTs that do not use this driver.
  if (pstPort == NULL)
    return;

  UartDmaPort_RxEventCallback(pstPort, huart, Size);
}

/**
 * @brief HAL TX complete callback dispatcher for registered UART DMA ports.
 *
 * @param[in] huart UART that completed transmission.
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  UartDmaPortTypedef *pstPort = FindUartDmaPort(huart);

  // Ignore callbacks from UARTs that do not use this driver.
  if (pstPort == NULL)
    return;

  UartDmaPort_TxCpltCallback(pstPort, huart);
}

/*=============================================================================
 * Private Function Definitions
 *============================================================================*/
/**
 * @brief Start receive-to-idle DMA on a UART DMA port.
 *
 * @param[in,out] pstPort_ UART DMA port object.
 *
 * @return HAL status.
 */
static HAL_StatusTypeDef EnableUartDmaPortReceive(UartDmaPortTypedef *pstPort_)
{
  HAL_StatusTypeDef eStatus = HAL_ERROR;

  // Reject incomplete receive configuration.
  if (IsUartDmaPortValid(pstPort_) == 0U)
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
static void TryStartUartDmaPortTx(UartDmaPortTypedef *pstPort_)
{
  uint16_t usLen = 0U;
  uint16_t usTail = 0U;
  uint32_t ulPrimask = 0U;

  // Reject incomplete port objects.
  if (IsUartDmaPortValid(pstPort_) == 0U)
    return;

  ulPrimask = EnterUartDmaPortCritical();

  // Nothing to start if DMA is busy or the ring is empty.
  if ((pstPort_->ucTxDmaActive != 0U) || (pstPort_->usTxHead == pstPort_->usTxTail))
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
  pstPort_->ucTxDmaActive = 1U;

  ExitUartDmaPortCritical(ulPrimask);

  // If HAL cannot start DMA now, release the active flag for retry later.
  if (StartUartDmaPortTransfer(pstPort_, &pstPort_->paucTxBuf[usTail], usLen) != HAL_OK)
  {
    ulPrimask = EnterUartDmaPortCritical();
    pstPort_->txDmaLen = 0U;
    pstPort_->ucTxDmaActive = 0U;
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
static uint16_t GetUartDmaPortTxFreeSpace(UartDmaPortTypedef *pstPort_)
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
 * @param[in] pstPort_  UART DMA port object.
 * @param[in] usIndex_  Current ring index.
 * @param[in] usCount_  Number of bytes to advance.
 *
 * @return Advanced ring index.
 */
static uint16_t AdvanceUartDmaPortIndex(UartDmaPortTypedef *pstPort_, uint16_t usIndex_, uint16_t usCount_)
{
  usIndex_ += usCount_;

  // Wrap the index back into the ring.
  while (usIndex_ >= pstPort_->usTxSize)
    usIndex_ -= pstPort_->usTxSize;

  return usIndex_;
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
    UartDmaPortTypedef *pstPort_,
    uint8_t *paucData_,
    uint16_t usLen_)
{
  // Reject invalid DMA transfer requests.
  if ((IsUartDmaPortValid(pstPort_) == 0U) || (paucData_ == NULL) || (usLen_ == 0U))
    return HAL_ERROR;

  // HAL owns the UART state while a TX DMA is in progress.
  if (pstPort_->pstHuart->gState != HAL_UART_STATE_READY)
    return HAL_BUSY;

  return HAL_UART_Transmit_DMA(pstPort_->pstHuart, paucData_, usLen_);
}

/**
 * @brief Check whether a UART DMA port object has the required fields.
 *
 * @param[in] pstPort_ UART DMA port object.
 *
 * @return 1 when valid, 0 otherwise.
 */
static uint8_t IsUartDmaPortValid(UartDmaPortTypedef *pstPort_)
{
  // Reject missing object pointers.
  if (pstPort_ == NULL)
    return 0U;

  // Reject incomplete UART/DMA buffer configuration.
  if ((pstPort_->pstHuart == NULL) ||
      (pstPort_->pstHdmaRx == NULL) ||
      (pstPort_->pstHdmaTx == NULL) ||
      (pstPort_->paucRxBuf == NULL) ||
      (pstPort_->paucTxBuf == NULL) ||
      (pstPort_->usRxSize == 0U) ||
      (pstPort_->usTxSize < 2U))
  {
    return 0U;
  }

  return 1U;
}

/**
 * @brief Register a UART DMA port for HAL callback dispatch.
 *
 * @param[in,out] pstPort_ UART DMA port object.
 *
 * @return HAL status.
 */
static HAL_StatusTypeDef RegisterUartDmaPort(UartDmaPortTypedef *pstPort_)
{
  uint16_t usIndex = 0U;

  // Reject incomplete port objects.
  if (IsUartDmaPortValid(pstPort_) == 0U)
    return HAL_ERROR;

  for (usIndex = 0U; usIndex < UART_DMA_PORT_MAX_PORTS; usIndex++)
  {
    // Already registered.
    if (apstTheUartDmaPorts[usIndex] == pstPort_)
      return HAL_OK;
  }

  for (usIndex = 0U; usIndex < UART_DMA_PORT_MAX_PORTS; usIndex++)
  {
    // Use the first empty registry slot.
    if (apstTheUartDmaPorts[usIndex] == NULL)
    {
      apstTheUartDmaPorts[usIndex] = pstPort_;
      return HAL_OK;
    }
  }

  return HAL_ERROR;
}

/**
 * @brief Find the registered UART DMA port for a HAL UART handle.
 *
 * @param[in] pstUart_ HAL UART handle.
 *
 * @return Matching port object, or NULL.
 */
static UartDmaPortTypedef *FindUartDmaPort(UART_HandleTypeDef *pstUart_)
{
  uint16_t usIndex = 0U;

  // Reject invalid UART handles.
  if (pstUart_ == NULL)
    return NULL;

  // 
  for (usIndex = 0U; usIndex < UART_DMA_PORT_MAX_PORTS; usIndex++)
  {
    // Match callbacks by HAL UART handle.
    if ((apstTheUartDmaPorts[usIndex] != NULL) && (apstTheUartDmaPorts[usIndex]->pstHuart == pstUart_))
      return apstTheUartDmaPorts[usIndex];
  }

  return NULL;
}
