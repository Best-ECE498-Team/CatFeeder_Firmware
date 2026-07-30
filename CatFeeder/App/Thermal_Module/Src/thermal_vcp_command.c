/******************************************************************************
 * @file    thermal_vcp_command.c
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
#include "thermal_vcp_command.h"
#include "../Internal/thermal_adc_driver.h"
#include "../Internal/ir_thermometer_driver.h"
#include "cmsis_os.h"
#include "vcp_debug.h"
#include <stdint.h>

/*=============================================================================
 * Private Macros
 *============================================================================*/
#define THERMAL_VCP_BURST_COUNT          (10U)
#define THERMAL_VCP_BURST_PRINT_COUNT    (10U)
#define THERMAL_VCP_PRINT_PERIOD_MS      (100U)
#define THERMAL_VCP_BURST_PERIOD_MS      (0U)

/*=============================================================================
 * Private Variables
 *============================================================================*/


/*=============================================================================
 * Private Function Prototypes
 *============================================================================*/
static void PrintThermalVcpBurst(void (*pfPrint_)(void));
static void PrintIrThermometerTemperatures(void);
static int16_t ThermalVcpFloatToDeciC(float fTemperatureC_);

/*=============================================================================
 * Public Function Definitions
 *============================================================================*/
/**
 * @brief Handle one VCP command for the System Manager module.
 *
 * @param[in] pstCmd_ Pointer to the VCP command structure
 *
 * @return true if command was recognized and handled, false otherwise
 */
bool HandleThermalVcpCommand(const VcpCommandTypeDef *pstCmd_)
{
  bool bHandled = false;

  if ((pstCmd_ == NULL) || (pstCmd_->pcAction == NULL))
    return false;

  // Handle "thermal.get" commands.
  if (IsVcpTokenEqual(pstCmd_->pcAction, "get") != false)
  {
    if (pstCmd_->ucArgc != 1U)
      return false;

    if (IsVcpTokenEqual(pstCmd_->pacArgv[0], "adcraw") != false)
    {
      PrintThermalVcpBurst(PrintThermalAdcRawValues);
      bHandled = true;
    }
    else if (IsVcpTokenEqual(pstCmd_->pacArgv[0], "adcval") != false)
    {
      PrintThermalVcpBurst(PrintThermalAdcValues);
      bHandled = true;
    }
    else if (IsVcpTokenEqual(pstCmd_->pacArgv[0], "adctemp") != false)
    {
      PrintThermalVcpBurst(PrintThermalAdcTemperatures);
      bHandled = true;
    }
    else if (IsVcpTokenEqual(pstCmd_->pacArgv[0], "irtemp") != false)
    {
      PrintThermalVcpBurst(PrintIrThermometerTemperatures);
      bHandled = true;
    }

    if (bHandled != false)
      DPRINTF_VCP("OK thermal get\r\n");

    return bHandled;
  }

  // Handle "thermal.help" command.
  if (IsVcpTokenEqual(pstCmd_->pcAction, "help") != false)
  {
    DPRINTF_VCP("Available commands:\r\n");
    DPRINTF_VCP("  thermal.get adcraw  - raw ADC1 values\r\n");
    DPRINTF_VCP("  thermal.get adcval  - filtered ADC1 values\r\n");
    DPRINTF_VCP("  thermal.get adctemp - ADC temperatures\r\n");
    DPRINTF_VCP("  thermal.get irtemp  - IR object/ambient temperatures\r\n");
    DPRINTF_VCP("  thermal.help\r\n");
    bHandled = true;

    return bHandled;
  }

  return false;
}

/*=============================================================================
 * Private Function Definitions
 *============================================================================*/
/**
 * @brief Print IR object and ambient temperatures for all thermal zones.
 */
static void PrintIrThermometerTemperatures(void)
{
  int16_t sHeaterObjDeciC = ThermalVcpFloatToDeciC(GetIrThermometerObjectTemperatureC(IR_THERMOMETER_HEATER));
  int16_t sHeaterAmbDeciC = ThermalVcpFloatToDeciC(GetIrThermometerAmbientTemperatureC(IR_THERMOMETER_HEATER));
  int16_t sCooler1ObjDeciC = ThermalVcpFloatToDeciC(GetIrThermometerObjectTemperatureC(IR_THERMOMETER_COOLER1));
  int16_t sCooler1AmbDeciC = ThermalVcpFloatToDeciC(GetIrThermometerAmbientTemperatureC(IR_THERMOMETER_COOLER1));
  int16_t sCooler2ObjDeciC = ThermalVcpFloatToDeciC(GetIrThermometerObjectTemperatureC(IR_THERMOMETER_COOLER2));
  int16_t sCooler2AmbDeciC = ThermalVcpFloatToDeciC(GetIrThermometerAmbientTemperatureC(IR_THERMOMETER_COOLER2));

  DPRINTF_TRACE(DBG_MASK_THERMAL, 
                "HEATER_IR_OBJ: %d.%01u C, HEATER_IR_AMB: %d.%01u C, COOLER1_IR_OBJ: %d.%01u C, COOLER1_IR_AMB: %d.%01u C, COOLER2_IR_OBJ: %d.%01u C, COOLER2_IR_AMB: %d.%01u C\r\n",
                (int32_t)(sHeaterObjDeciC / 10),
                (uint32_t)((sHeaterObjDeciC < 0) ? (-sHeaterObjDeciC % 10) : (sHeaterObjDeciC % 10)),
                (int32_t)(sHeaterAmbDeciC / 10),
                (uint32_t)((sHeaterAmbDeciC < 0) ? (-sHeaterAmbDeciC % 10) : (sHeaterAmbDeciC % 10)),
                (int32_t)(sCooler1ObjDeciC / 10),
                (uint32_t)((sCooler1ObjDeciC < 0) ? (-sCooler1ObjDeciC % 10) : (sCooler1ObjDeciC % 10)),
                (int32_t)(sCooler1AmbDeciC / 10),
                (uint32_t)((sCooler1AmbDeciC < 0) ? (-sCooler1AmbDeciC % 10) : (sCooler1AmbDeciC % 10)),
                (int32_t)(sCooler2ObjDeciC / 10),
                (uint32_t)((sCooler2ObjDeciC < 0) ? (-sCooler2ObjDeciC % 10) : (sCooler2ObjDeciC % 10)),
                (int32_t)(sCooler2AmbDeciC / 10),
                (uint32_t)((sCooler2AmbDeciC < 0) ? (-sCooler2AmbDeciC % 10) : (sCooler2AmbDeciC % 10)));
}

/**
 * @brief Convert a Celsius float to rounded deci-Celsius for VCP printing.
 */
static int16_t ThermalVcpFloatToDeciC(float fTemperatureC_)
{
  if (fTemperatureC_ >= 0.0f)
    return (int16_t)((fTemperatureC_ * 10.0f) + 0.5f);
  else
    return (int16_t)((fTemperatureC_ * 10.0f) - 0.5f);
}

static void PrintThermalVcpBurst(void (*pfPrint_)(void))
{
  if (pfPrint_ == NULL)
    return;

  for (uint8_t ucBurst = 0U; ucBurst < THERMAL_VCP_BURST_COUNT; ucBurst++)
  {
    DPRINTF_VCP("Burst: %u\r\n", (uint32_t)(ucBurst + 1U));
    for (uint8_t ucPrint = 0U; ucPrint < THERMAL_VCP_BURST_PRINT_COUNT; ucPrint++)
    {
      pfPrint_();
      if (ucPrint < (THERMAL_VCP_BURST_PRINT_COUNT - 1U))
        osDelay(THERMAL_VCP_PRINT_PERIOD_MS);
    }

    osDelay(THERMAL_VCP_BURST_PERIOD_MS);
  }
}
