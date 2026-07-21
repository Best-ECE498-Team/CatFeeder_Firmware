/******************************************************************************
 * @file    error_handler.h
 * @brief   Header file for error handling functions and fault logging.
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

#ifndef INC_ERROR_HANDLER_H_
#define INC_ERROR_HANDLER_H_

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * Includes
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include "stm32g4xx.h"

/*=============================================================================
 * Public Macros
 *============================================================================*/
// SysFault flags definitions
#define SYSFAULT_OS_MALLOC_FAILED       (1U << 1)
#define SYSFAULT_OS_STACK_OVERFLOW      (1U << 2)
#define SYSFAULT_VCP_INIT               (1U << 3)
#define SYSFAULT_ASSERT_FAIL            (1U << 4)
#define SYSFAULT_HAL_ERROR              (1U << 5)

// Define a magic number to identify valid fault backup log
#define FAULT_BACKUP_MAGIC_NUMBER       (0xFA17FA17U)

// Define a pointer to the fault backup registers in the backup register space
#define FAULT_BKPR                      ((volatile FaultBackupRegsTypeDef *)(TAMP_BASE + 0x100U))

/**
 * @brief      The assert_param macro is used for function's parameters check.
 * @param[in]  expr_ If expr is false, it calls HandleAssertFail function
 *             If expr is true, it returns no value.
 * @retval     None
 */
#define ASSERT(expr_)                                       \
  do                                                        \
  {                                                         \
    expr_ ? (void)0 : HandleAssertFail(__FILE__, __LINE__); \
  } while (0)

/**
 * @brief   Capture the application call site and trigger the critical error handler, no return.
 *
 * @details This is for software-detected critical errors, not CPU exception entry.
 *          It stores the current PC, LR, and SP into FAULT_BKPR before calling
 *          HandleCriticalError(NULL, 0U).
 */
#define HANDLE_CRITICAL_ERROR()                                 \
  do                                                            \
  {                                                             \
    FAULT_BKPR->MAGIC = 0;                                      \
    uint32_t ulFaultPc;                                         \
    uint32_t ulFaultLr;                                         \
    uint32_t ulFaultSp;                                         \
    __asm volatile                                              \
    (                                                           \
      "mov %0, pc\n"                                            \
      "mov %1, lr\n"                                            \
      "mov %2, sp\n"                                            \
      : "=r" (ulFaultPc), "=r" (ulFaultLr), "=r" (ulFaultSp)    \
    );                                                          \
    FAULT_BKPR->SP = ulFaultSp;                                 \
    FAULT_BKPR->LR = ulFaultLr;                                 \
    FAULT_BKPR->PC = ulFaultPc;                                 \
    FAULT_BKPR->EXC_RETURN = 0U;                                \
    FAULT_BKPR->MAGIC = FAULT_BACKUP_MAGIC_NUMBER;              \
    HandleCriticalError(NULL, 0U);                              \
  } while (0)

/**
 * @brief Load the SCB (System Control Block) fault registers into the fault backup structure for later analysis.
 */
#define BACKUP_SCB_FAULT_REGISTERS()               \
  do                                               \
  {                                                \
    FAULT_BKPR->MAGIC = 0;                         \
    FAULT_BKPR->CFSR = SCB->CFSR;                  \
    FAULT_BKPR->HFSR = SCB->HFSR;                  \
    FAULT_BKPR->MMFAR = SCB->MMFAR;                \
    FAULT_BKPR->BFAR = SCB->BFAR;                  \
    FAULT_BKPR->AFSR = SCB->AFSR;                  \
    FAULT_BKPR->MAGIC = FAULT_BACKUP_MAGIC_NUMBER; \
  } while (0) 

/*=============================================================================
 * Public Type Definitions
 *============================================================================*/
/**
 * @brief      Define backup registers for fault logging.
 * 
 * @details    This structure is used to store the CPU registers and fault status registers at the time of a fault. 
 *             It is placed in the backup registers of the STM32G4 microcontroller to retain the information across system resets.
 * 
 * @attention  The structure must be aligned to 4 bytes and must not exceed max of 32 registers available in STM32G474ret 
 *             Current address offset [0x100 , 0x140].
 */
typedef struct
{
  //CPU registers at the time of fault
  __IO uint32_t MAGIC;          /**< Magic number,                               Address offset: 0x100 */
  
  __IO uint32_t R0;             /**< Function argument 1,                        Address offset: 0x104 */
  __IO uint32_t R1;             /**< Function argument 2,                        Address offset: 0x108 */
  __IO uint32_t R2;             /**< Function argument 3,                        Address offset: 0x10C */
  __IO uint32_t R3;             /**< Function argument 4,                        Address offset: 0x110 */
  __IO uint32_t IP;             /**< R12 Intra-procedure-call scratch register,  Address offset: 0x114 */
  __IO uint32_t SP;             /**< R13 Stack pointer,                          Address offset: 0x118 */
  __IO uint32_t LR;             /**< R14 Link register,                          Address offset: 0x11C */
  __IO uint32_t PC;             /**< R15 Program counter,                        Address offset: 0x120 */
  __IO uint32_t xPSR;           /**< Special Program status register,            Address offset: 0x124 */
  __IO uint32_t EXC_RETURN;     /**< Exception return value,                     Address offset: 0x128 */

  // System Control Block (SCB) registers for fault status
  __IO uint32_t CFSR;           /**< Configurable fault status register,         Address offset: 0x12C */
  __IO uint32_t HFSR;           /**< Hard fault status register,                 Address offset: 0x130 */
  __IO uint32_t MMFAR;          /**< Memory management fault address register,   Address offset: 0x134 */
  __IO uint32_t BFAR;           /**< Bus fault address register,                 Address offset: 0x138 */
  __IO uint32_t AFSR;           /**< Auxiliary fault status register,            Address offset: 0x13C */

  // Platform-specific fault data
  __IO uint32_t SysFaultFlags;  /**< System fault flags,                         Address offset: 0x140 */
} FaultBackupRegsTypeDef;

/*=============================================================================
 * Public Constants
 *============================================================================*/


/*=============================================================================
 * Public Function Prototypes
 *============================================================================*/
void HandleCriticalError(uint32_t *pulStackFrame_, uint32_t ulExcReturn_) __attribute__((noreturn));
void HandleAssertFail(uint8_t *pucFile_, uint32_t ulLine_);
void LoadCpuFaultRegisters(uint32_t *pulStackFrame_, uint32_t ulExcReturn_);
const char* GetResetCauseString(void);
void PrintFaultBackupReport(void);

/**
 * 
 */

#ifdef __cplusplus
}
#endif

#endif /* INC_ERROR_HANDLER_H_ */
