/******************************************************************************
 * @file    error_handler.c
 * @brief   Handler for system reset and fault conditions.
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
#include "error_handler.h"
#include "usart.h"
#include <stdbool.h>
#include <stdio.h>

/*=============================================================================
 * Private Macros
 *============================================================================*/
#define IS_MEMORY_FAULT_ADDRESS_VALID() (((FAULT_BKPR->CFSR & SCB_CFSR_MMARVALID_Msk) != 0U))
#define IS_BUS_FAULT_ADDRESS_VALID()    (((FAULT_BKPR->CFSR & SCB_CFSR_BFARVALID_Msk) != 0U))
#define ASSERT_FAIL_PRINT_BUF_SIZE       (160U)
#define EMERGENCY_UART_FLAG_TIMEOUT      (1000000U)

/*=============================================================================
 * Private Type Definitions
 *============================================================================*/
typedef struct
{
  uint32_t ulMask;
  const char *pcDescription;
} FaultDecodeEntryTypeDef;

/**
  * @brief Possible STM32 system reset causes
  */
typedef enum
{
  RESET_CAUSE_UNKNOWN = 0U,
  RESET_CAUSE_LOW_POWER_RESET,
  RESET_CAUSE_WINDOW_WATCHDOG_RESET,
  RESET_CAUSE_INDEPENDENT_WATCHDOG_RESET,
  RESET_CAUSE_SOFTWARE_RESET,
  RESET_CAUSE_OPTION_BYTE_LOADER_RESET,
  RESET_CAUSE_EXTERNAL_RESET_PIN_RESET,
  RESET_CAUSE_BROWNOUT_RESET,
} ResetCauseTypeDef;

/*=============================================================================
 * Private Variables
 *============================================================================*/;
static const FaultDecodeEntryTypeDef astSysFaultDecodeTable[] =
{
  { SYSFAULT_OS_MALLOC_FAILED,  "FreeRTOS malloc failed" },
  { SYSFAULT_OS_STACK_OVERFLOW, "FreeRTOS stack overflow detected" },
  { SYSFAULT_VCP_INIT,          "VCP debug initialization failed" },
  { SYSFAULT_ASSERT_FAIL,       "Application assert failed" },
  { SYSFAULT_HAL_ERROR,         "HAL error occurred" },
};

static const FaultDecodeEntryTypeDef astHardFaultDecodeTable[] =
{
  { SCB_HFSR_VECTTBL_Msk,  "Bus error on vector table read (VECTTBL)" },
  { SCB_HFSR_FORCED_Msk,   "Configurable fault escalated to hard fault (FORCED)" },
  { SCB_HFSR_DEBUGEVT_Msk, "Debug event occurred (DEBUGEVT)" },
};

static const FaultDecodeEntryTypeDef astMemoryFaultDecodeTable[] =
{
  { SCB_CFSR_IACCVIOL_Msk,  "Instruction access violation (IACCVIOL)" },
  { SCB_CFSR_DACCVIOL_Msk,  "Data access violation (DACCVIOL)" },
  { SCB_CFSR_MUNSTKERR_Msk, "Unstacking error (MUNSTKERR)" },
  { SCB_CFSR_MSTKERR_Msk,   "Stacking error (MSTKERR)" },
  { SCB_CFSR_MLSPERR_Msk,   "Floating-point lazy state preservation error (MLSPERR)" },
};

static const FaultDecodeEntryTypeDef astBusFaultDecodeTable[] =
{
  { SCB_CFSR_IBUSERR_Msk,     "Instruction bus error (IBUSERR)" },
  { SCB_CFSR_PRECISERR_Msk,   "Precise data bus error (PRECISERR)" },
  { SCB_CFSR_IMPRECISERR_Msk, "Imprecise data bus error (IMPRECISERR)" },
  { SCB_CFSR_UNSTKERR_Msk,    "Unstacking error (UNSTKERR)" },
  { SCB_CFSR_STKERR_Msk,      "Stacking error (STKERR)" },
  { SCB_CFSR_LSPERR_Msk,      "Floating-point lazy state preservation error (LSPERR)" },
};

static const FaultDecodeEntryTypeDef astUsageFaultDecodeTable[] =
{
  { SCB_CFSR_UNDEFINSTR_Msk, "Attempt to execute an undefined instruction (UNDEFINSTR)" },
  { SCB_CFSR_INVSTATE_Msk,   "Attempt to enter an invalid instruction set state (INVSTATE)" },
  { SCB_CFSR_INVPC_Msk,      "Invalid EXC_RETURN value (INVPC)" },
  { SCB_CFSR_NOCP_Msk,       "Attempt to access a coprocessor (NOCP)" },
  { SCB_CFSR_UNALIGNED_Msk,  "Illegal unaligned load or store (UNALIGNED)" },
  { SCB_CFSR_DIVBYZERO_Msk,  "Divide by zero (DIVBYZERO)" },
};

static const FaultDecodeEntryTypeDef astXpsrDecodeTable[] =
{
  { xPSR_V_Msk, "Overflow condition code flag set (V)" },
  { xPSR_T_Msk, "Thumb state bit set (T), inspect LR for corrupted fn return" },
};

/*=============================================================================
 * Private Function Prototypes
 *============================================================================*/
ResetCauseTypeDef GetResetCause(void);
static void PrintXpsrDetails(uint32_t ulXpsr_);
static void PrintFaultDecodeTable(const char *pcTitle_,
                                  uint32_t ulRegValue_,
                                  const FaultDecodeEntryTypeDef *pastTable_,
                                  uint32_t ulTableCount_);
static inline void ClearFaultBackupRegisters(void);

/*=============================================================================
 * Public Function Definitions
 *============================================================================*/
/**
 * @brief     Handle assert failure by logging the location and triggering a system reset.
 * 
 * @param[in]  pucFile_  Pointer to the file name string
 * @param[in]  ulLine_   Line number where the assert failed
 */
void HandleAssertFail(uint8_t *pucFile_, uint32_t ulLine_)
{
  uint32_t ulDelayCycles = 10000000U;

  // (TODO) Disable all actuators and power electronics to prevent further execution and potential damage

  // Fatal logging owns the debug UART from here until reset.
  __disable_irq(); 

  FAULT_BKPR->SysFaultFlags |= SYSFAULT_ASSERT_FAIL;
  FAULT_BKPR->MAGIC = FAULT_BACKUP_MAGIC_NUMBER;

  printf("[ERROR] [SYSTEM] ASSERT FAILED: file %s, line %lu\r\n", 
          (const char *)pucFile_, (uint32_t)ulLine_);

  // Delay 1 second
  while (ulDelayCycles-- > 0U)
  {
    __NOP();
  }

  // Trigger a software system reset
  __DSB();
  __ISB();
  NVIC_SystemReset();

  // Trap if reset does not start.
  while(1)
  {

  }
}

/**
 * @brief     Handle critical error then trigger a system reset, no return.
 * 
 * @attention This function is called with argments in ARMv7-M Thumb "startup_stm32g474retx.s", find more detail there.
 *            Application code should not call this directly, use HANDLE_CRITICAL_ERROR() macro.
 * 
 * @param[in]  pulStackFrame_  Pointer to the exception stack frame
 * @param[in]  ulExcReturn_    Exception return value
 */
__attribute__((noreturn)) 
void HandleCriticalError(uint32_t *pulStackFrame_, uint32_t ulExcReturn_)
{
  // Load CPU registers from the stack frame and exception return value into the fault backup structure
  LoadCpuFaultRegisters(pulStackFrame_, ulExcReturn_);

  BACKUP_SCB_FAULT_REGISTERS();

  // (TODO) force GPIOs/MOSFETs/heater/TEC/fans etc to safe state using direct register writes

  // Disable interrupts to prevent further execution.
  __disable_irq(); 

  // Indicate a valid fault if any SysFaultFlags is set.
  if (FAULT_BKPR->SysFaultFlags != 0)
    FAULT_BKPR->MAGIC = FAULT_BACKUP_MAGIC_NUMBER;

  // Delay 1 second
  uint32_t ulDelayCycles = 10000000; 
  while (ulDelayCycles-- > 0U)
  {
    __NOP();
  }

  // Trigger a software system reset
  __DSB();
  __ISB();
  NVIC_SystemReset();

  // Should not reach here, but if it does, disable interrupts and enter an infinite loop.
  // Trap if reset does not start.
  while(1)
  {

  }
}

/**
 * @brief      Load CPU fault registers from the exception stack pointer and exception return value 
 *             into the fault backup structure.
 *             Reference PM0214 programming manual Figure 12. Cortex-M4 stack frame layout.
 * 
 * @param[in]  pulStackFrame_  Pointer to the exception stack frame
 * @param[in]  ulExcReturn_    Exception return value
 */
void LoadCpuFaultRegisters(uint32_t *pulStackFrame_, uint32_t ulExcReturn_)
{
  // Only accepting exception stack frame.
  // Skip CPU register capture for software-triggered faults.
  if (pulStackFrame_ == NULL)
    return;

  FAULT_BKPR->MAGIC = 0;

  // Load CPU registers from the exception stack frame.
  FAULT_BKPR->SP = (uint32_t)pulStackFrame_; 
  FAULT_BKPR->EXC_RETURN = ulExcReturn_;
  FAULT_BKPR->R0 = pulStackFrame_[0];
  FAULT_BKPR->R1 = pulStackFrame_[1];
  FAULT_BKPR->R2 = pulStackFrame_[2];
  FAULT_BKPR->R3 = pulStackFrame_[3];
  FAULT_BKPR->IP = pulStackFrame_[4];
  FAULT_BKPR->LR = pulStackFrame_[5];
  FAULT_BKPR->PC = pulStackFrame_[6];
  FAULT_BKPR->xPSR = pulStackFrame_[7];

  // Set the magic number to indicate valid fault backup log.
  FAULT_BKPR->MAGIC = FAULT_BACKUP_MAGIC_NUMBER;
}

/**
 * @brief     Obtain the system reset cause as an ASCII-printable name string
 *            from a reset cause type
 * 
 * @return    A ASCII name string describing the system reset cause
 */
const char* GetResetCauseString(void)
{
  ResetCauseTypeDef eResetCause = GetResetCause();

  const char *pucResetCauseName = "TBD";

  // Convert the reset cause enum to a printable string.
  switch (eResetCause) 
  {
    case RESET_CAUSE_UNKNOWN:
      pucResetCauseName = "UNKNOWN";
      break;
    case RESET_CAUSE_LOW_POWER_RESET:
      pucResetCauseName = "LOW_POWER_RESET";
      break;
    case RESET_CAUSE_WINDOW_WATCHDOG_RESET:
      pucResetCauseName = "WINDOW_WATCHDOG_RESET";
      break;
    case RESET_CAUSE_INDEPENDENT_WATCHDOG_RESET:
      pucResetCauseName = "INDEPENDENT_WATCHDOG_RESET";
      break;
    case RESET_CAUSE_SOFTWARE_RESET:
      pucResetCauseName = "SOFTWARE_RESET";
      break;
    case RESET_CAUSE_OPTION_BYTE_LOADER_RESET:
      pucResetCauseName = "OPTION_BYTE_LOADER_RESET";
      break;
    case RESET_CAUSE_EXTERNAL_RESET_PIN_RESET:
      pucResetCauseName = "EXTERNAL_RESET_PIN_RESET";
      break;
    case RESET_CAUSE_BROWNOUT_RESET:
      pucResetCauseName = "BROWNOUT_RESET (BOR)";
      break;
  }

  return pucResetCauseName;
}

/**
 * @brief  Print the backed-up fault register report over blocking printf().
 */
void PrintFaultBackupReport(void)
{
  // Print normal startup when no valid backup exists.
  if (FAULT_BKPR->MAGIC != FAULT_BACKUP_MAGIC_NUMBER)
  {
    printf("* == Normal Startup == \r\n");
    return;
  }
  else
  {
    // Print exception startup when a backup exists.
    printf("* !== Exception Startup ==!\r\n");
  }

  printf("* !============================== Fault Backup Report ==============================! \r\n");
  printf("*  |R0: 0x%08lX   |R1: 0x%08lX   |R2: 0x%08lX   |R3: 0x%08lX\r\n",
      (uint32_t)FAULT_BKPR->R0,
      (uint32_t)FAULT_BKPR->R1,
      (uint32_t)FAULT_BKPR->R2,
      (uint32_t)FAULT_BKPR->R3);
  printf("*  |IP: 0x%08lX   |SP: 0x%08lX   |LR: 0x%08lX   |PC: 0x%08lX\r\n",
      (uint32_t)FAULT_BKPR->IP,
      (uint32_t)FAULT_BKPR->SP,
      (uint32_t)FAULT_BKPR->LR,
      (uint32_t)FAULT_BKPR->PC);
  printf("*  |xPSR: 0x%08lX       |EXC_RETURN: 0x%08lX       |AFSR: 0x%08lX\r\n",
      (uint32_t)FAULT_BKPR->xPSR,
      (uint32_t)FAULT_BKPR->EXC_RETURN,
      (uint32_t)FAULT_BKPR->AFSR);
  printf("*  |CFSR: 0x%08lX       |HFSR: 0x%08lX             |SysFaultFlags: 0x%08lX\r\n",
      (uint32_t)FAULT_BKPR->CFSR,
      (uint32_t)FAULT_BKPR->HFSR,
      (uint32_t)FAULT_BKPR->SysFaultFlags);

  PrintFaultDecodeTable(
      "Platform System Fault Details",
      FAULT_BKPR->SysFaultFlags,
      astSysFaultDecodeTable,
      (uint32_t)(sizeof(astSysFaultDecodeTable) / sizeof(astSysFaultDecodeTable[0])));

  PrintXpsrDetails(FAULT_BKPR->xPSR);

  PrintFaultDecodeTable(
      "Hard Fault Details",
      FAULT_BKPR->HFSR,
      astHardFaultDecodeTable,
      (uint32_t)(sizeof(astHardFaultDecodeTable) / sizeof(astHardFaultDecodeTable[0])));

  PrintFaultDecodeTable(
      "Memory Management Fault Details",
      FAULT_BKPR->CFSR,
      astMemoryFaultDecodeTable,
      (uint32_t)(sizeof(astMemoryFaultDecodeTable) / sizeof(astMemoryFaultDecodeTable[0])));

  // Print MMFAR only when the hardware marks it valid.
  if (IS_MEMORY_FAULT_ADDRESS_VALID())
    printf("*   Memory management fault address register (MMFAR): 0x%08lX\r\n", (uint32_t)FAULT_BKPR->MMFAR);
  else
    printf("*   Memory management fault address register (MMFAR): not valid\r\n");

  PrintFaultDecodeTable(
      "Bus Fault Details",
      FAULT_BKPR->CFSR,
      astBusFaultDecodeTable,
      (uint32_t)(sizeof(astBusFaultDecodeTable) / sizeof(astBusFaultDecodeTable[0])));

  // Print BFAR only when the hardware marks it valid.
  if (IS_BUS_FAULT_ADDRESS_VALID())
    printf("*   Bus fault address register (BFAR): 0x%08lX\r\n", (uint32_t)FAULT_BKPR->BFAR);
  else
    printf("*   Bus fault address register (BFAR): not valid\r\n");

  PrintFaultDecodeTable(
      "Usage Fault Details",
      FAULT_BKPR->CFSR,
      astUsageFaultDecodeTable,
      (uint32_t)(sizeof(astUsageFaultDecodeTable) / sizeof(astUsageFaultDecodeTable[0])));
  
  // (TODO) Investigate better reseting behavior
  ClearFaultBackupRegisters();
  printf("* Fault backup registers cleared\r\n");
  printf("* !=================================================================================! \r\n");
}

/*=============================================================================
 * Private Function Definitions
 *============================================================================*/
/**
 * @brief      Print decoded xPSR fields from a backed-up xPSR value.
 * 
 * @param[in]  ulXpsr_ Backed-up xPSR value
 */
static void PrintXpsrDetails(uint32_t ulXpsr_)
{
  xPSR_Type stXpsr;
  const char *pcExceptionName = "External interrupt";
  uint32_t ulExceptionNumber = 0U;

  stXpsr.w = ulXpsr_;
  ulExceptionNumber = (stXpsr.w & xPSR_ISR_Msk) >> xPSR_ISR_Pos;

  // Decode the exception number field.
  switch (ulExceptionNumber)
  {
    case 0U:
      pcExceptionName = "Thread mode";
      break;

    case 1U:
      pcExceptionName = "Reset";
      break;

    case 2U:
      pcExceptionName = "Non-maskable interrupt";
      break;

    case 3U:
      pcExceptionName = "Hard fault";
      break;

    case 4U:
      pcExceptionName = "Memory management fault";
      break;

    case 5U:
      pcExceptionName = "Bus fault";
      break;

    case 6U:
      pcExceptionName = "Usage fault";
      break;

    case 11U:
      pcExceptionName = "SVCall";
      break;

    case 12U:
      pcExceptionName = "Debug monitor";
      break;

    case 14U:
      pcExceptionName = "PendSV";
      break;

    case 15U:
      pcExceptionName = "SysTick";
      break;

    default:
      pcExceptionName = "External interrupt";
      break;
  }

  printf("* xPSR Details\r\n");
  printf("*   Exception number (ISR): %lu", (unsigned long)ulExceptionNumber);

  // Print IRQn only for external interrupts.
  if (ulExceptionNumber >= 16U)
    printf("*   (%s IRQn=%ld)\r\n", pcExceptionName, (long)(ulExceptionNumber - 16U));
  else
    // Print core exception name without IRQn.
    printf("*   (%s)\r\n", pcExceptionName);

  PrintFaultDecodeTable(
      "xPSR Condition/State Flags",
      ulXpsr_,
      astXpsrDecodeTable,
      (uint32_t)(sizeof(astXpsrDecodeTable) / sizeof(astXpsrDecodeTable[0])));
}

/**
 * @brief      Print every active flag from a fault decode table.
 * 
 * @param[in]  paucTitle_  Report section title
 * @param[in]  ulRegValue_ Register or flag value to decode
 * @param[in]  pastTable_  Decode table
 * @param[in]  ulTableCount_ Number of entries in the decode table
 */
static void PrintFaultDecodeTable(const char *paucTitle_, 
                                  uint32_t ulRegValue_,
                                  const FaultDecodeEntryTypeDef *pastTable_,
                                  uint32_t ulTableCount_)
{
  bool bAnySet = false;
  uint32_t ulIndex = 0U;

  printf("* %s\r\n", paucTitle_);

  // Scan every decode-table entry.
  for (ulIndex = 0U; ulIndex < ulTableCount_; ulIndex++)
  {
    // Print entries whose bit is set.
    if ((ulRegValue_ & pastTable_[ulIndex].ulMask) != 0U)
    {
      printf("*   %s\r\n", pastTable_[ulIndex].pcDescription);
      bAnySet = true;
    }
  }

  // Print empty result when no bits matched.
  if (bAnySet == false)
    printf("*   None\r\n");
}

/**
 * @brief   Obtain the STM32 system reset cause
 * 
 * @return  The system reset cause
 */
ResetCauseTypeDef GetResetCause(void)
{
  ResetCauseTypeDef eResetCause = RESET_CAUSE_UNKNOWN;

  // Check the RCC reset flags to determine the cause of the last system reset
  if (RCC->CSR & RCC_CSR_LPWRRSTF) 
    eResetCause = RESET_CAUSE_LOW_POWER_RESET;
  else if (RCC->CSR & RCC_CSR_WWDGRSTF) 
    eResetCause = RESET_CAUSE_WINDOW_WATCHDOG_RESET;
  else if (RCC->CSR & RCC_CSR_IWDGRSTF) 
    eResetCause = RESET_CAUSE_INDEPENDENT_WATCHDOG_RESET;
  else if (RCC->CSR & RCC_CSR_SFTRSTF) 
    eResetCause = RESET_CAUSE_SOFTWARE_RESET;
  else if (RCC->CSR & RCC_CSR_PINRSTF) 
    eResetCause = RESET_CAUSE_EXTERNAL_RESET_PIN_RESET;
  else if (RCC->CSR & RCC_CSR_BORRSTF) 
    eResetCause = RESET_CAUSE_BROWNOUT_RESET;
  else if (RCC->CSR & RCC_CSR_OBLRSTF) 
    eResetCause = RESET_CAUSE_OPTION_BYTE_LOADER_RESET;
  else 
    eResetCause = RESET_CAUSE_UNKNOWN;

  // Clear all the reset flags or else they will remain set
  RCC->CSR |= RCC_CSR_RMVF;

  return eResetCause;
}

/**
 * @brief Clear all fault backup registers.
 */
static inline void ClearFaultBackupRegisters(void)
{
  volatile uint32_t *pulReg = (volatile uint32_t *)FAULT_BKPR;

  // Clear all fault backup registers to zero.
  for (uint32_t ulIndex = 0U;
       ulIndex < (sizeof(FaultBackupRegsTypeDef) / sizeof(uint32_t));
       ulIndex++)
    pulReg[ulIndex] = 0U;
}
