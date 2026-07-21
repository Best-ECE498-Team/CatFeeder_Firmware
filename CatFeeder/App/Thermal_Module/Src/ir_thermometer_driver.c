/******************************************************************************
 * @file    ir_thermometer_driver.c
 * @brief   
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
#include "..\Internal\ir_thermometer_driver.h"
#include "i2c.h"
#include "error_handler.h"
#include "vcp_debug.h"
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "utilities.h"
#include "cmsis_os.h"

/*=============================================================================
 * Private Macros
 *============================================================================*/
// Default MLX90614 7-bit SMBus slave address. HAL I2C APIs expect it left shifted by 1.
#define MLX90614_SMBUS_ADDR_7BIT              0x5AU
#define MLX90614_SMBUS_ADDR_HAL               (MLX90614_SMBUS_ADDR_7BIT << 1U)

// MLX90614 SMBus command opcodes.
#define MLX90614_CMD_RAM_ACCESS               0x00U
#define MLX90614_CMD_EEPROM_ACCESS            0x20U
#define MLX90614_CMD_READ_FLAGS               0xF0U
#define MLX90614_CMD_SLEEP                    0xFFU
#define MLX90614_CMD_RAM_READ(reg_)           (MLX90614_CMD_RAM_ACCESS | ((reg_) & MLX90614_RAM_ADDR_BIT_MASK))
#define MLX90614_CMD_EEPROM_READ(reg_)        (MLX90614_CMD_EEPROM_ACCESS | ((reg_) & MLX90614_EEPROM_ADDR_BIT_MASK))
#define MLX90614_CMD_EEPROM_WRITE(reg_)       (MLX90614_CMD_EEPROM_ACCESS | ((reg_) & MLX90614_EEPROM_ADDR_BIT_MASK))

// MLX90614 customer EEPROM register addresses.
#define MLX90614_EEPROM_TOBJ_MAX_ADDR         0x00U
#define MLX90614_EEPROM_TOBJ_MIN_ADDR         0x01U
#define MLX90614_EEPROM_PWM_CTRL_ADDR         0x02U
#define MLX90614_EEPROM_TA_RANGE_ADDR         0x03U
#define MLX90614_EEPROM_EMISSIVITY_ADDR       0x04U
#define MLX90614_EEPROM_CONFIG_REG_1_ADDR     0x05U
#define MLX90614_EEPROM_SMBUS_ADDR            0x0EU
#define MLX90614_EEPROM_ID_NUMBER_1_ADDR      0x1CU
#define MLX90614_EEPROM_ID_NUMBER_2_ADDR      0x1DU
#define MLX90614_EEPROM_ID_NUMBER_3_ADDR      0x1EU
#define MLX90614_EEPROM_ID_NUMBER_4_ADDR      0x1FU
#define MLX90614_EEPROM_ADDR_BIT_MASK         MLX90614_EEPROM_ID_NUMBER_4_ADDR

// MLX90614 RAM register addresses.
#define MLX90614_RAM_RAW_IR_CH1_ADDR          0x04U
#define MLX90614_RAM_RAW_IR_CH2_ADDR          0x05U
#define MLX90614_RAM_AMBIENT_TEMP_ADDR        0x06U
#define MLX90614_RAM_OBJECT_TEMP_1_ADDR       0x07U
#define MLX90614_RAM_OBJECT_TEMP_2_ADDR       0x08U
#define MLX90614_RAM_ADDR_BIT_MASK            0x1FU

// MLX90614 Config Register 1 bit fields.
#define MLX90614_CONFIG1_IIR_SHIFT            0U
#define MLX90614_CONFIG1_IIR_MASK             (0x7U << MLX90614_CONFIG1_IIR_SHIFT)
#define MLX90614_CONFIG1_REPEAT_TEST          (1U << 3)
#define MLX90614_CONFIG1_PWM_MODE_SHIFT       4U
#define MLX90614_CONFIG1_PWM_MODE_MASK        (0x3U << MLX90614_CONFIG1_PWM_MODE_SHIFT)
#define MLX90614_CONFIG1_DUAL_IR_SENSOR       (1U << 6)
#define MLX90614_CONFIG1_KS_NEGATIVE          (1U << 7)
#define MLX90614_CONFIG1_FIR_SHIFT            8U
#define MLX90614_CONFIG1_FIR_MASK             (0x7U << MLX90614_CONFIG1_FIR_SHIFT)
#define MLX90614_CONFIG1_GAIN_SHIFT           11U
#define MLX90614_CONFIG1_GAIN_MASK            (0x7U << MLX90614_CONFIG1_GAIN_SHIFT)
#define MLX90614_CONFIG1_KT2_NEGATIVE         (1U << 14)
#define MLX90614_CONFIG1_SELFTEST_DISABLED    (1U << 15)

// MLX90614 Config Register 1 IIR filter settings.
#define MLX90614_CONFIG1_IIR_50_PERCENT       (0x0U << MLX90614_CONFIG1_IIR_SHIFT)
#define MLX90614_CONFIG1_IIR_25_PERCENT       (0x1U << MLX90614_CONFIG1_IIR_SHIFT)
#define MLX90614_CONFIG1_IIR_17_PERCENT       (0x2U << MLX90614_CONFIG1_IIR_SHIFT)
#define MLX90614_CONFIG1_IIR_13_PERCENT       (0x3U << MLX90614_CONFIG1_IIR_SHIFT)
#define MLX90614_CONFIG1_IIR_100_PERCENT      (0x4U << MLX90614_CONFIG1_IIR_SHIFT)
#define MLX90614_CONFIG1_IIR_80_PERCENT       (0x5U << MLX90614_CONFIG1_IIR_SHIFT)
#define MLX90614_CONFIG1_IIR_67_PERCENT       (0x6U << MLX90614_CONFIG1_IIR_SHIFT)
#define MLX90614_CONFIG1_IIR_57_PERCENT       (0x7U << MLX90614_CONFIG1_IIR_SHIFT)

// MLX90614 Config Register 1 PWM output selection.
#define MLX90614_CONFIG1_PWM_TA_TOBJ1         (0x0U << MLX90614_CONFIG1_PWM_MODE_SHIFT)
#define MLX90614_CONFIG1_PWM_TA_TOBJ2         (0x1U << MLX90614_CONFIG1_PWM_MODE_SHIFT)
#define MLX90614_CONFIG1_PWM_TOBJ2_ONLY       (0x2U << MLX90614_CONFIG1_PWM_MODE_SHIFT)
#define MLX90614_CONFIG1_PWM_TOBJ1_TOBJ2      (0x3U << MLX90614_CONFIG1_PWM_MODE_SHIFT)

// MLX90614 Config Register 1 FIR filter settings.
#define MLX90614_CONFIG1_FIR_8                (0x0U << MLX90614_CONFIG1_FIR_SHIFT)
#define MLX90614_CONFIG1_FIR_16               (0x1U << MLX90614_CONFIG1_FIR_SHIFT)
#define MLX90614_CONFIG1_FIR_32               (0x2U << MLX90614_CONFIG1_FIR_SHIFT)
#define MLX90614_CONFIG1_FIR_64               (0x3U << MLX90614_CONFIG1_FIR_SHIFT)
#define MLX90614_CONFIG1_FIR_128              (0x4U << MLX90614_CONFIG1_FIR_SHIFT)
#define MLX90614_CONFIG1_FIR_256              (0x5U << MLX90614_CONFIG1_FIR_SHIFT)
#define MLX90614_CONFIG1_FIR_512              (0x6U << MLX90614_CONFIG1_FIR_SHIFT)
#define MLX90614_CONFIG1_FIR_1024             (0x7U << MLX90614_CONFIG1_FIR_SHIFT)

// MLX90614 Config Register 1 IR amplifier gain settings.
#define MLX90614_CONFIG1_GAIN_1               (0x0U << MLX90614_CONFIG1_GAIN_SHIFT)
#define MLX90614_CONFIG1_GAIN_3               (0x1U << MLX90614_CONFIG1_GAIN_SHIFT)
#define MLX90614_CONFIG1_GAIN_6               (0x2U << MLX90614_CONFIG1_GAIN_SHIFT)
#define MLX90614_CONFIG1_GAIN_12_5            (0x3U << MLX90614_CONFIG1_GAIN_SHIFT)
#define MLX90614_CONFIG1_GAIN_25              (0x4U << MLX90614_CONFIG1_GAIN_SHIFT)
#define MLX90614_CONFIG1_GAIN_50              (0x5U << MLX90614_CONFIG1_GAIN_SHIFT)
#define MLX90614_CONFIG1_GAIN_100             (0x6U << MLX90614_CONFIG1_GAIN_SHIFT)
#define MLX90614_CONFIG1_GAIN_100_ALT         (0x7U << MLX90614_CONFIG1_GAIN_SHIFT)

// MLX90614 flags returned by the read-flags command.
#define MLX90614_FLAGS_ALWAYS_ZERO_MASK       0xFF01U
#define MLX90614_FLAGS_NOT_IMPLEMENTED_MASK   0x004EU
#define MLX90614_FLAGS_INIT_COMPLETE          (1U << 4)
#define MLX90614_FLAGS_EE_DEAD                (1U << 5)
#define MLX90614_FLAGS_EEPROM_BUSY            (1U << 7)

// Temperature RAM words use bit 15 as an error flag and 0.02 K per LSB.
#define MLX90614_TEMP_ERROR_FLAG              (1U << 15)
#define MLX90614_TEMP_DATA_MASK               0x7FFFU
#define MLX90614_TEMP_LSB_CENTIK              2U
#define MLX90614_KELVIN_OFFSET_CENTIC         27315U

// MLX90614 SMBus timing limits.
#define MLX90614_SMBUS_MIN_FREQ_HZ            10000U
#define MLX90614_SMBUS_MAX_FREQ_HZ            100000U
#define MLX90614_DATA_READY_AFTER_POR_TYP_MS  250U
#define MLX90614_DATA_READY_AFTER_POR_WAIT_MS 300U
#define MLX90614_WAKEUP_SDA_LOW_MIN_MS        33U
#define MLX90614_WAKEUP_SDA_LOW_WAIT_MS       35U
#define MLX90614_EEPROM_WRITE_DELAY_MS        10U
#define MLX90614_EEPROM_ERASE_DELAY_MS        10U
#define MLX90614_I2C_TIMEOUT_MS               100U
#define MLX90614_I2C_READY_TRIALS             3U
#define MLX90614_SMBUS_PEC_POLY               0x07U
#define MLX90614_READ_WORD_DATA_SIZE          3U
#define MLX90614_WRITE_WORD_DATA_SIZE         3U

// Applicaiton DSP configuration
#define MLX90614_CONFIG1_FILTER_MASK          (MLX90614_CONFIG1_IIR_MASK | MLX90614_CONFIG1_FIR_MASK)
#define MLX90614_CONFIG1_FILTER_SETTING       (MLX90614_CONFIG1_IIR_100_PERCENT | MLX90614_CONFIG1_FIR_1024)

/*=============================================================================
 * Private Type Definitions
 *============================================================================*/
typedef struct
{
  I2C_HandleTypeDef *pstI2c;
  const char *pcIdString;
}IrThermometerTypeDef;

/*=============================================================================
 * Private Variables
 *============================================================================*/
static const IrThermometerTypeDef astTheIrThermometers[] =
{
  { &hi2c1, "Heater IRT" },    // Heater IR thermometer
  { &hi2c3, "Cooler1 IRT" },   // Cooler1 IR thermometer
  { &hi2c4, "Cooler2 IRT" },   // Cooler2 IR thermometer
};

/*=============================================================================
 * Private Function Prototypes
 *============================================================================*/
static bool IrThermometerInit(const IrThermometerTypeDef *pstIrThermometer_);
#if 0
// Read-flags support requires custom timing and will be enabled later.
static bool IrThermometerCheckFlags(uint16_t usFlags_);
static bool IrThermometerReadFlags(I2C_HandleTypeDef *pstI2c_,
                                   uint16_t *pusFlags_);
#endif
static bool IrThermometerVerifyConfig(I2C_HandleTypeDef *pstI2c_);
static bool IrThermometerVerifyTemperatureRead(I2C_HandleTypeDef *pstI2c_);
static float IrThermometerGetTemperatureC(IrThermometerIdTypeDef eIrThermometer_,
                                          uint8_t ucRamReg_);
static bool IrThermometerReadWord(I2C_HandleTypeDef *pstI2c_,
                                  uint8_t ucCommand_,
                                  uint16_t *pusData_);
static bool IrThermometerWriteWord(I2C_HandleTypeDef *pstI2c_,
                                   uint8_t ucCommand_,
                                   uint16_t usData_);
static float IrThermometerRawToTemperatureC(uint16_t usRawTemperature_);
static uint8_t IrThermometerCalcPec(const uint8_t *pucData_,
                                    uint8_t ucDataSize_);


/*=============================================================================
 * Public Function Definitions
 *============================================================================*/
/**
 * @brief   Initialize all MLX90614 IR thermometers.
 * 
 * @details Checks the heater, cooler1, and cooler2 IR thermometers on I2C1, I2C3,
 *          and I2C4. Each sensor must accept the expected Config Register 1
 *          filter settings and return valid ambient/object temperature words.
 *          Read-flags is not working because the MLX90614 flags command i  s a
 *          special SMBus transaction.
 */
void IrThermometersInit(void)
{
  osDelay(MLX90614_DATA_READY_AFTER_POR_WAIT_MS);

  // Initialize each configured IR thermometer bus.
  for (uint8_t i = 0U; i < (uint8_t)(ARRAY_SIZE(astTheIrThermometers)); i++)
  {
    // Log init failures.
    // Handle failures (TODO).
    if (IrThermometerInit(&astTheIrThermometers[i]) == false)
      DPRINTF_ERROR(DBG_MASK_THERMAL, "%s INIT FAIL\r\n", astTheIrThermometers[i].pcIdString);
    else
      DPRINTF_INFO(DBG_MASK_THERMAL, "%s Initialized\r\n", astTheIrThermometers[i].pcIdString);
  }
}

/**
 * @brief     Get MLX90614 object temperature in Celsius.
 * 
 * @param[in] eIrThermometer_ IR thermometer selection
 * 
 * @return    Object temperature in Celsius, or IR_THERMOMETER_INVALID_TEMPERATURE_C on failure
 */
float GetIrThermometerObjectTemperatureC(IrThermometerIdTypeDef eIrThermometer_)
{
  return IrThermometerGetTemperatureC(eIrThermometer_,
                                      MLX90614_RAM_OBJECT_TEMP_1_ADDR);
}

/**
 * @brief     Get MLX90614 ambient temperature in Celsius.
 * 
 * @param[in] eIrThermometer_ IR thermometer selection
 * 
 * @return    Ambient temperature in Celsius, or IR_THERMOMETER_INVALID_TEMPERATURE_C on failure
 */
float GetIrThermometerAmbientTemperatureC(IrThermometerIdTypeDef eIrThermometer_)
{
  return IrThermometerGetTemperatureC(eIrThermometer_,
                                      MLX90614_RAM_AMBIENT_TEMP_ADDR);
}

/*=============================================================================
 * Private Function Definitions
 *============================================================================*/
/**
 * @brief     Initialize one MLX90614 sensor.
 * 
 * @param[in] pstIrThermometer_ IR thermometer instance
 * 
 * @return    True if the sensor passed the startup checks
 */
static bool IrThermometerInit(const IrThermometerTypeDef *pstIrThermometer_)
{
  ASSERT(pstIrThermometer_ != NULL);
  ASSERT(pstIrThermometer_->pstI2c != NULL);

  // Confirm that the sensor acknowledges its default SMBus address.
  if (HAL_I2C_IsDeviceReady(pstIrThermometer_->pstI2c,
                            MLX90614_SMBUS_ADDR_HAL,
                            MLX90614_I2C_READY_TRIALS,
                            MLX90614_I2C_TIMEOUT_MS) != HAL_OK)
  {                            
    DPRINTF_ERROR(DBG_MASK_THERMAL, "%s DEVICE NOT READY (disconnected?)\r\n", pstIrThermometer_->pcIdString);
    return false;
  }
  else
  {
    DPRINTF_DEBUG(DBG_MASK_THERMAL, "%s device ready\r\n", pstIrThermometer_->pcIdString);
  }
  
  // Read-flags command requires custom timing and will be added later. (TODO)
  DPRINTF_WARN(DBG_MASK_THERMAL, "%s DEVICE FLAG CHECK SKIPPED\r\n", pstIrThermometer_->pcIdString);

  // Verify the required DSP filter configuration.
  if (IrThermometerVerifyConfig(pstIrThermometer_->pstI2c) == false)
  {
    DPRINTF_ERROR(DBG_MASK_THERMAL, "%s DSP CONFIG EEPROM FAILED\r\n", pstIrThermometer_->pcIdString);
    return false;
  }
  else
  {
    DPRINTF_DEBUG(DBG_MASK_THERMAL, "%s DSP config1 register: 0x%X \r\n", pstIrThermometer_->pcIdString, 
                  MLX90614_CONFIG1_FILTER_SETTING);
    DPRINTF_DEBUG(DBG_MASK_THERMAL, "%s DSP config eeprom programmed\r\n", pstIrThermometer_->pcIdString);
  }

  // Confirm that the sensor returns valid temperature words.
  if (IrThermometerVerifyTemperatureRead(pstIrThermometer_->pstI2c) == false)
  {
    DPRINTF_ERROR(DBG_MASK_THERMAL, "%s TEMPERATURE ERROR\r\n", pstIrThermometer_->pcIdString);
    return false;
  }
  else
  {
    DPRINTF_DEBUG(DBG_MASK_THERMAL, "%s temperature verified\r\n", pstIrThermometer_->pcIdString);
  }

  return true;
}

/**
 * @brief     Check the MLX90614 read-flags word.
 * 
 * @param[in] usFlags_ Flags word returned by MLX90614_CMD_READ_FLAGS
 * 
 * @return    True if the sensor finished POR and EEPROM is usable
 */
#if 0
static bool IrThermometerCheckFlags(uint16_t usFlags_)
{
  // POR initialization must be complete before the first measurement checks.
  if ((usFlags_ & MLX90614_FLAGS_INIT_COMPLETE) == 0U)
    return false;

  // EEPROM writes/erases must not still be in progress.
  if ((usFlags_ & MLX90614_FLAGS_EEPROM_BUSY) != 0U)
    return false;

  // A dead EEPROM flag means the sensor calibration data is not trustworthy.
  if ((usFlags_ & MLX90614_FLAGS_EE_DEAD) != 0U)
    return false;

  return true;
}
#endif

/**
 * @brief     Verify and update MLX90614 Config Register 1 filter settings.
 * 
 * @details   Only the IIR and FIR fields are changed. Other Config Register 1
 *            fields are preserved.
 * 
 * @param[in] pstI2c_ I2C handle connected to one MLX90614 sensor
 * 
 * @return    True if the EEPROM contains the expected filter settings
 */
static bool IrThermometerVerifyConfig(I2C_HandleTypeDef *pstI2c_)
{
  uint16_t usConfig = 0U;
  uint16_t usNewConfig = 0U;

  ASSERT(pstI2c_ != NULL);

  // Read the current EEPROM configuration before deciding whether to write.
  if (IrThermometerReadWord(pstI2c_,
                            MLX90614_CMD_EEPROM_READ(MLX90614_EEPROM_CONFIG_REG_1_ADDR),
                            &usConfig) == false)
    return false;

  // Keep EEPROM untouched when the filter fields already match.
  if ((usConfig & MLX90614_CONFIG1_FILTER_MASK) == MLX90614_CONFIG1_FILTER_SETTING)
    return true;

  usNewConfig = (uint16_t)((usConfig & ~MLX90614_CONFIG1_FILTER_MASK) |
                           MLX90614_CONFIG1_FILTER_SETTING);

  // MLX90614 EEPROM cells must be erased before programming the new value.
  // Erase Config Register 1 before writing the updated filter fields.
  if (IrThermometerWriteWord(pstI2c_,
                             MLX90614_CMD_EEPROM_WRITE(MLX90614_EEPROM_CONFIG_REG_1_ADDR),
                             0x0000U) == false)
    return false;

  osDelay(MLX90614_EEPROM_ERASE_DELAY_MS);

  // Program Config Register 1 with only the filter bits changed.
  if (IrThermometerWriteWord(pstI2c_,
                             MLX90614_CMD_EEPROM_WRITE(MLX90614_EEPROM_CONFIG_REG_1_ADDR),
                             usNewConfig) == false)
    return false;

  osDelay(MLX90614_EEPROM_WRITE_DELAY_MS);

  // Read back EEPROM to confirm the new filter setting was accepted.
  if (IrThermometerReadWord(pstI2c_,
                            MLX90614_CMD_EEPROM_READ(MLX90614_EEPROM_CONFIG_REG_1_ADDR),
                            &usConfig) == false)
    return false;

  return ((usConfig & MLX90614_CONFIG1_FILTER_MASK) == MLX90614_CONFIG1_FILTER_SETTING);
}

/**
 * @brief     Test read the ambient and object temperature registers.
 * 
 * @param[in] pstI2c_ I2C handle connected to one MLX90614 sensor
 * 
 * @return    True if both temperature words are valid
 */
static bool IrThermometerVerifyTemperatureRead(I2C_HandleTypeDef *pstI2c_)
{
  uint16_t usAmbientTemp = 0U;
  uint16_t usObjectTemp = 0U;

  ASSERT(pstI2c_ != NULL);

  // Read ambient temperature as the first measurement sanity check.
  if (IrThermometerReadWord(pstI2c_,
                            MLX90614_CMD_RAM_READ(MLX90614_RAM_AMBIENT_TEMP_ADDR),
                            &usAmbientTemp) == false)
    return false;

  // Temperature bit 15 indicates an MLX90614 measurement error.
  if ((usAmbientTemp & MLX90614_TEMP_ERROR_FLAG) != 0U)
    return false;

  // Read object temperature after ambient has passed.
  if (IrThermometerReadWord(pstI2c_,
                            MLX90614_CMD_RAM_READ(MLX90614_RAM_OBJECT_TEMP_1_ADDR),
                            &usObjectTemp) == false)
    return false;

  // Temperature bit 15 indicates an MLX90614 measurement error.
  if ((usObjectTemp & MLX90614_TEMP_ERROR_FLAG) != 0U)
    return false;

  return true;
}

/**
 * @brief     Read one MLX90614 temperature RAM register.
 * 
 * @param[in] eIrThermometer_ IR thermometer selection
 * @param[in] ucRamReg_       Temperature RAM register address
 * 
 * @return    Temperature in Celsius, or IR_THERMOMETER_INVALID_TEMPERATURE_C on failure
 */
static float IrThermometerGetTemperatureC(IrThermometerIdTypeDef eIrThermometer_,
                                          uint8_t ucRamReg_)
{
  uint16_t usRawTemperature = 0U;

  // Reject invalid sensor selections.
  if (eIrThermometer_ >= IR_THERMOMETER_COUNT)
    return IR_THERMOMETER_INVALID_TEMPERATURE_C;

  // Read the selected MLX90614 temperature RAM word.
  if (IrThermometerReadWord(astTheIrThermometers[eIrThermometer_].pstI2c,
                            MLX90614_CMD_RAM_READ(ucRamReg_),
                            &usRawTemperature) == false)
    return IR_THERMOMETER_INVALID_TEMPERATURE_C;

  // Temperature bit 15 indicates an MLX90614 measurement error.
  if ((usRawTemperature & MLX90614_TEMP_ERROR_FLAG) != 0U)
    return IR_THERMOMETER_INVALID_TEMPERATURE_C;

  return IrThermometerRawToTemperatureC(usRawTemperature);
}

/**
 * @brief      Read the MLX90614 flags word.
 * 
 * @param[in]  pstI2c_   I2C handle connected to one MLX90614 sensor
 * @param[out] pusFlags_ Read flags word
 * 
 * @return     True if the flags word was read
 */
#if 0
static bool IrThermometerReadFlags(I2C_HandleTypeDef *pstI2c_,
                                   uint16_t *pusFlags_)
{
  ASSERT(pstI2c_ != NULL);
  ASSERT(pusFlags_ != NULL);

  // Read flags through the same command path as read-word transactions.
  return IrThermometerReadWord(pstI2c_,
                               MLX90614_CMD_READ_FLAGS,
                               pusFlags_);
}
#endif

/**
 * @brief      Read one SMBus word and verify PEC.
 * 
 * @param[in]  pstI2c_    I2C handle connected to one MLX90614 sensor
 * @param[in]  ucCommand_ SMBus command byte
 * @param[out] pusData_   Read word
 * 
 * @return     True if the read and PEC check passed
 */
static bool IrThermometerReadWord(I2C_HandleTypeDef *pstI2c_,
                                  uint8_t ucCommand_,
                                  uint16_t *pusData_)
{
  ASSERT(pstI2c_ != NULL);
  ASSERT(pusData_ != NULL);

  uint8_t aucRxData[MLX90614_READ_WORD_DATA_SIZE] = {0U};
  uint8_t aucPecData[5U] = {0U};

  // Read word commands return LSB, MSB, then PEC.
  if (HAL_I2C_Mem_Read(pstI2c_,
                       MLX90614_SMBUS_ADDR_HAL,
                       ucCommand_,
                       I2C_MEMADD_SIZE_8BIT,
                       aucRxData,
                       MLX90614_READ_WORD_DATA_SIZE,
                       MLX90614_I2C_TIMEOUT_MS) != HAL_OK)
    return false;

  aucPecData[0] = MLX90614_SMBUS_ADDR_HAL;
  aucPecData[1] = ucCommand_;
  aucPecData[2] = (MLX90614_SMBUS_ADDR_HAL | 0x01U);
  aucPecData[3] = aucRxData[0];
  aucPecData[4] = aucRxData[1];

  // Reject frames with an invalid SMBus PEC byte.
  if (IrThermometerCalcPec(aucPecData, (uint8_t)sizeof(aucPecData)) != aucRxData[2])
  {
    DPRINTF_ERROR(DBG_MASK_THERMAL,
                  "MLX90614 PEC FAIL CMD=0x%02X RX=%02X %02X %02X CALC=%02X\r\n",
                  ucCommand_,
                  aucRxData[0],
                  aucRxData[1],
                  aucRxData[2],
                  IrThermometerCalcPec(aucPecData, (uint8_t)sizeof(aucPecData)));
    return false;
  }

  *pusData_ = (uint16_t)(((uint16_t)aucRxData[1] << 8U) | aucRxData[0]);
  return true;
}

/**
 * @brief     Write one SMBus word with PEC.
 * 
 * @param[in] pstI2c_    I2C handle connected to one MLX90614 sensor
 * @param[in] ucCommand_ SMBus command byte
 * @param[in] usData_    Data word to write
 * 
 * @return    True if the write completed
 */
static bool IrThermometerWriteWord(I2C_HandleTypeDef *pstI2c_,
                                   uint8_t ucCommand_,
                                   uint16_t usData_)
{
  ASSERT(pstI2c_ != NULL);

  uint8_t aucTxData[MLX90614_WRITE_WORD_DATA_SIZE] = {0U};
  uint8_t aucPecData[4U] = {0U};

  aucTxData[0] = (uint8_t)(usData_ & 0xFFU);
  aucTxData[1] = (uint8_t)((usData_ >> 8U) & 0xFFU);

  aucPecData[0] = MLX90614_SMBUS_ADDR_HAL;
  aucPecData[1] = ucCommand_;
  aucPecData[2] = aucTxData[0];
  aucPecData[3] = aucTxData[1];
  aucTxData[2] = IrThermometerCalcPec(aucPecData, (uint8_t)sizeof(aucPecData));

  // Write word commands send LSB, MSB, then PEC.
  return (HAL_I2C_Mem_Write(pstI2c_,
                            MLX90614_SMBUS_ADDR_HAL,
                            ucCommand_,
                            I2C_MEMADD_SIZE_8BIT,
                            aucTxData,
                            MLX90614_WRITE_WORD_DATA_SIZE,
                            MLX90614_I2C_TIMEOUT_MS) == HAL_OK);
}

/**
 * @brief     Convert MLX90614 temperature word to Celsius.
 * 
 * @details   MLX90614 temperature words use 0.02 K per LSB.
 * 
 * @param[in] usRawTemperature_ Raw MLX90614 temperature word
 * 
 * @return    Temperature in Celsius
 */
static float IrThermometerRawToTemperatureC(uint16_t usRawTemperature_)
{
  int32_t lTemperatureCentiC = 0;

  lTemperatureCentiC = ((int32_t)(usRawTemperature_ & MLX90614_TEMP_DATA_MASK) *
                        (int32_t)MLX90614_TEMP_LSB_CENTIK) -
                       (int32_t)MLX90614_KELVIN_OFFSET_CENTIC;

  return ((float)lTemperatureCentiC / 100.0f);
}

/**
 * @brief     Calculate SMBus PEC CRC-8.
 * 
 * @param[in] pucData_    PEC input bytes
 * @param[in] ucDataSize_ Number of PEC input bytes
 * 
 * @return    PEC byte
 */
static uint8_t IrThermometerCalcPec(const uint8_t *pucData_,
                                    uint8_t ucDataSize_)
{
  ASSERT(pucData_ != NULL);

  uint8_t ucPec = 0U;

  // Accumulate every byte into the CRC-8 PEC state.
  for (uint8_t i = 0U; i < ucDataSize_; i++)
  {
    ucPec ^= pucData_[i];

    // Shift one byte through the SMBus PEC polynomial.
    for (uint8_t j = 0U; j < 8U; j++)
    {
      // Apply the polynomial when the outgoing CRC bit is set.
      if ((ucPec & 0x80U) != 0U)
        ucPec = (uint8_t)((ucPec << 1U) ^ MLX90614_SMBUS_PEC_POLY);
      else
        // Shift only when no polynomial tap is needed.
        ucPec <<= 1U;
    }
  }

  return ucPec;
}
