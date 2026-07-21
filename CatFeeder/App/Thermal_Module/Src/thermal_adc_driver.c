/******************************************************************************
 * @file    thermal_adc_driver.c
 * @brief   Thermal ADC driver implementation
 *
 * @project PawPlate - Intelligent Wet Cat Food Dispensing System
 * @course  ECE 498 Engineering Design Project
 * @team    Team 53
 * @authors Bowen Zheng
 *          Jingyan Xu
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
#include "..\Internal\thermal_adc_driver.h"
#include "adc.h"
#include "tim.h"
#include "error_handler.h"
#include "vcp_debug.h"
#include "utilities.h"
#include "cmsis_os.h"

/*=============================================================================
 * Private Macros
 *============================================================================*/
// Number of ADC1 channels used for thermal module 
#define ADC1_CHANNEL_COUNT          ((ADC1->SQR1 & ADC_SQR1_L) + 1U) 

// DSP filters setting
#define THERMAL_ADC_FRAME_SIZE      16U  // Averaging frame
#define THERMAL_ADC_AVG_SHIFT       4U   // log2(16) = 4
// Fix point IIR filter parameters, cutoff frequency = 10Hz, sampling frequency = 1kHz.
#define THERMAL_ADC_IIR_ALPHA_SHIFT 3U  // IIR alpha = 1/8 = 0.062 ~= alpha = 1 - e^(-2πfc*(1/fs) ) = 0.061
#define THERMAL_ADC_Q_SHIFT         8U  // Fractional bits used by the IIR accumulator

#if !IS_POWER_OF_TWO(1U << THERMAL_ADC_IIR_ALPHA_SHIFT)
  #error "THERMAL_ADC_IIR_ALPHA_SHIFT must be a power of two"
#endif
#if !IS_POWER_OF_TWO(1U << THERMAL_ADC_Q_SHIFT)
  #error "THERMAL_ADC_Q_SHIFT must be a power of two"
#endif
#if THERMAL_ADC_FRAME_SIZE != (1U << THERMAL_ADC_AVG_SHIFT)
  #error "THERMAL_ADC_FRAME_SIZE must match 1 << THERMAL_ADC_AVG_SHIFT"
#endif
#if !IS_POWER_OF_TWO(THERMAL_ADC_FRAME_SIZE)
  #error "THERMAL_ADC_FRAME_SIZE must be a power of two"
#endif

#define ADC1_INVIL_MARGIN           15U
#define NTC_ADC_OPEN_CIRCUIT_THRESH 4085U
#define NTC_ADC_GND_SHORT_THRESH    10U
/**
 * @brief     ADC1 thermal channel fields in DMA rank order.
 *
 * @attention Keep this field order synchronized with the ADC1 regular channel ranks in CubeMX. 
 *            The uint16_t version is used directly as the DMA frame layout.
 */
#define ADC1_VAL_FIELDS(type) \
  type ulHeaterNtc;           \
  type ulCooler1Ntc;          \
  type ulCooler2Ntc;          \
  type ulCooler1HsNtc;        \
  type ulCooler2HsNtc;        \
  type ulMcuTemp;             \
  type ulVrefint

/*=============================================================================
 * Private Type Definitions
 *============================================================================*/
/**
 * @brief ADC1 Fileds.
 */
typedef struct
{
  ADC1_VAL_FIELDS(uint16_t);
}Adc1ValTypeDef;

typedef struct
{
  ADC1_VAL_FIELDS(uint32_t);
}ADC1AccumValTypeDef;

/**
 * @brief ADC1 filtering state.
 */
typedef struct
{
  Adc1ValTypeDef stVals;                  /**< Latest filtered ADC values */
  ADC1AccumValTypeDef stFilterAccVals;    /**< Fixed-point IIR accumulator values */
  bool bFilterInitialized;                /**< True after the IIR state is seeded */
}Adc1FilterStateTypeDef;

typedef struct
{
  uint16_t usAdcVal;
  int16_t sTemperatureC;
}NtcLookUpTableTypeDef;

/*=============================================================================
 * Private Variables
 *============================================================================*/
// Circular dma buffer to hold the last 10 frames of ADC1 raw values for thermal sensors 
static Adc1ValTypeDef astTheAdc1RawVals[THERMAL_ADC_FRAME_SIZE];

// Last completed ADC1 raw DMA frame.
static Adc1ValTypeDef stTheAdc1RawSnapshot;

// Filtered ADC output and fixed-point IIR state. 
static Adc1FilterStateTypeDef stTheAdc1FilterState;

/**
 * @brief   Loop up table for SunLord SDNT2012X103F3950FTF NTC thermistor, R25=10kΩ, B25/50=3950K
 * 
 * @details The table is generated based on the R25 and B25/85 parameters and provides a mapping from 
 *          ADC values to temperature in Celsius.
 *          The ADC values are based on a 12-bit ADC with a reference voltage of 3.3V and a voltage divider with a 10kΩ series resistor.
 */
static const NtcLookUpTableTypeDef astSunLordNtcLookUpTable[] =
{
  {4063, -55}, {4061, -54}, {4058, -53}, {4055, -52}, {4051, -51},
  {4048, -50}, {4044, -49}, {4040, -48}, {4036, -47}, {4031, -46},
  {4026, -45}, {4021, -44}, {4015, -43}, {4009, -42}, {4002, -41},
  {3996, -40}, {3988, -39}, {3981, -38}, {3972, -37}, {3964, -36},
  {3955, -35}, {3945, -34}, {3935, -33}, {3924, -32}, {3912, -31},
  {3900, -30}, {3887, -29}, {3874, -28}, {3860, -27}, {3845, -26},
  {3830, -25}, {3813, -24}, {3796, -23}, {3778, -22}, {3760, -21},
  {3740, -20}, {3720, -19}, {3698, -18}, {3676, -17}, {3653, -16},
  {3629, -15}, {3604, -14}, {3578, -13}, {3551, -12}, {3524, -11},
  {3495, -10}, {3465,  -9}, {3435,  -8}, {3403,  -7}, {3371,  -6},
  {3337,  -5}, {3303,  -4}, {3267,  -3}, {3231,  -2}, {3194,  -1},
  {3156,   0},
  {3118,   1}, {3078,   2}, {3038,   3}, {2997,   4}, {2955,   5},
  {2913,   6}, {2870,   7}, {2826,   8}, {2782,   9}, {2738,  10},
  {2693,  11}, {2648,  12}, {2602,  13}, {2556,  14}, {2510,  15},
  {2464,  16}, {2417,  17}, {2371,  18}, {2324,  19}, {2278,  20},
  {2231,  21}, {2185,  22}, {2139,  23}, {2093,  24}, {2048,  25},
  {2002,  26}, {1957,  27}, {1913,  28}, {1868,  29}, {1825,  30},
  {1781,  31}, {1739,  32}, {1697,  33}, {1655,  34}, {1614,  35},
  {1574,  36}, {1534,  37}, {1495,  38}, {1456,  39}, {1419,  40},
  {1382,  41}, {1346,  42}, {1310,  43}, {1275,  44}, {1241,  45},
  {1208,  46}, {1175,  47}, {1143,  48}, {1112,  49}, {1081,  50},
  {1052,  51}, {1023,  52}, { 994,  53}, { 967,  54}, { 940,  55},
  { 914,  56}, { 888,  57}, { 863,  58}, { 839,  59}, { 815,  60},
  { 792,  61}, { 770,  62}, { 748,  63}, { 727,  64}, { 707,  65},
  { 687,  66}, { 668,  67}, { 649,  68}, { 631,  69}, { 613,  70},
  { 596,  71}, { 579,  72}, { 563,  73}, { 547,  74}, { 532,  75},
  { 517,  76}, { 502,  77}, { 488,  78}, { 475,  79}, { 462,  80},
  { 449,  81}, { 436,  82}, { 424,  83}, { 413,  84}, { 401,  85},
  { 390,  86}, { 380,  87}, { 369,  88}, { 359,  89}, { 350,  90},
  { 340,  91}, { 331,  92}, { 322,  93}, { 314,  94}, { 305,  95},
  { 297,  96}, { 289,  97}, { 282,  98}, { 274,  99}, { 267, 100},
  { 260, 101}, { 253, 102}, { 247, 103}, { 240, 104}, { 234, 105},
  { 228, 106}, { 222, 107}, { 217, 108}, { 211, 109}, { 206, 110},
  { 201, 111}, { 196, 112}, { 191, 113}, { 186, 114}, { 181, 115},
  { 177, 116}, { 173, 117}, { 168, 118}, { 164, 119}, { 160, 120},
  { 156, 121}, { 153, 122}, { 149, 123}, { 145, 124}, { 142, 125},
  { 138, 126}, { 135, 127}, { 132, 128}, { 129, 129}, { 126, 130},
  { 123, 131}, { 120, 132}, { 117, 133}, { 115, 134}, { 112, 135},
  { 110, 136}, { 107, 137}, { 105, 138}, { 102, 139}, { 100, 140},
  {  98, 141}, {  96, 142}, {  93, 143}, {  91, 144}, {  89, 145},
  {  87, 146}, {  86, 147}, {  84, 148}, {  82, 149}, {  80, 150},
};

/**
 * @brief   Loop up table for Vishay SNTCALUG01A103J NTC thermistor, R25=10kΩ, B25/85=3984K
 * 
 * @details The table is generated based on the vender's lab RT measurements and provides a mapping 
 *          from ADC values to temperature in Celsius.
 *          The ADC values are based on a 12-bit ADC with a reference voltage of 3.3V and a voltage divider with a 10kΩ series resistor.
 */
static const NtcLookUpTableTypeDef astVishayNtcLookUpTable[] =
{
  {4053, -55}, {4049, -54}, {4046, -53}, {4042, -52}, {4038, -51},
  {4034, -50}, {4030, -49}, {4025, -48}, {4020, -47}, {4015, -46},
  {4009, -45}, {4003, -44}, {3997, -43}, {3991, -42}, {3984, -41},
  {3976, -40}, {3968, -39}, {3960, -38}, {3951, -37}, {3942, -36},
  {3932, -35}, {3922, -34}, {3911, -33}, {3900, -32}, {3888, -31},
  {3875, -30}, {3862, -29}, {3848, -28}, {3833, -27}, {3818, -26},
  {3802, -25}, {3786, -24}, {3768, -23}, {3750, -22}, {3731, -21},
  {3711, -20}, {3691, -19}, {3669, -18}, {3647, -17}, {3624, -16},
  {3600, -15}, {3575, -14}, {3550, -13}, {3523, -12}, {3496, -11},
  {3467, -10}, {3438,  -9}, {3408,  -8}, {3376,  -7}, {3344,  -6},
  {3312,  -5}, {3278,  -4}, {3243,  -3}, {3208,  -2}, {3171,  -1},
  {3134,   0},
  {3096,   1}, {3058,   2}, {3018,   3}, {2978,   4}, {2938,   5},
  {2896,   6}, {2854,   7}, {2812,   8}, {2769,   9}, {2725,  10},
  {2681,  11}, {2637,  12}, {2592,  13}, {2547,  14}, {2502,  15},
  {2457,  16}, {2411,  17}, {2366,  18}, {2320,  19}, {2274,  20},
  {2229,  21}, {2183,  22}, {2138,  23}, {2093,  24}, {2048,  25},
  {2003,  26}, {1958,  27}, {1914,  28}, {1870,  29}, {1827,  30},
  {1784,  31}, {1742,  32}, {1700,  33}, {1658,  34}, {1618,  35},
  {1577,  36}, {1538,  37}, {1499,  38}, {1460,  39}, {1423,  40},
  {1386,  41}, {1349,  42}, {1314,  43}, {1279,  44}, {1244,  45},
  {1211,  46}, {1178,  47}, {1146,  48}, {1114,  49}, {1084,  50},
  {1054,  51}, {1024,  52}, { 996,  53}, { 968,  54}, { 941,  55},
  { 914,  56}, { 888,  57}, { 863,  58}, { 839,  59}, { 815,  60},
  { 792,  61}, { 769,  62}, { 747,  63}, { 726,  64}, { 705,  65},
  { 685,  66}, { 665,  67}, { 646,  68}, { 627,  69}, { 609,  70},
  { 592,  71}, { 575,  72}, { 558,  73}, { 542,  74}, { 527,  75},
  { 512,  76}, { 497,  77}, { 483,  78}, { 469,  79}, { 456,  80},
  { 443,  81}, { 430,  82}, { 418,  83}, { 406,  84}, { 395,  85},
  { 383,  86}, { 373,  87}, { 362,  88}, { 352,  89}, { 342,  90},
  { 333,  91}, { 323,  92}, { 314,  93}, { 306,  94}, { 297,  95},
  { 289,  96}, { 281,  97}, { 273,  98}, { 266,  99}, { 259, 100},
  { 252, 101}, { 245, 102}, { 238, 103}, { 232, 104}, { 226, 105},
  { 219, 106}, { 214, 107}, { 208, 108}, { 202, 109}, { 197, 110},
  { 192, 111}, { 187, 112}, { 182, 113}, { 177, 114}, { 173, 115},
  { 168, 116}, { 164, 117}, { 160, 118}, { 156, 119}, { 152, 120},
  { 148, 121}, { 144, 122}, { 140, 123}, { 137, 124}, { 133, 125},
  { 130, 126}, { 127, 127}, { 124, 128}, { 121, 129}, { 118, 130},
  { 115, 131}, { 112, 132}, { 109, 133}, { 107, 134}, { 104, 135},
  { 102, 136}, {  99, 137}, {  97, 138}, {  94, 139}, {  92, 140},
  {  90, 141}, {  88, 142}, {  86, 143}, {  84, 144}, {  82, 145},
  {  80, 146}, {  78, 147}, {  76, 148}, {  75, 149}, {  73, 150},
};

/*=============================================================================
 * Private Function Prototypes
 *============================================================================*/
static void ThermalAdcConvCompleteCallback(ADC_HandleTypeDef *hadc);
static inline void ThermalAdcUpdateFilteredValues(void);
static inline uint16_t Get1stOrderIIRFilteredVal(uint16_t usVal_, 
                                                 uint32_t *pulAccVal_);
static bool ThermalAdcVerifyNtcReadings(const Adc1ValTypeDef *pstAdc1Vals_);
static bool ThermalAdcVerifyNtcReading(const char *pcNtcName_, 
                                       uint16_t usAdcVal_,
                                       const NtcLookUpTableTypeDef *pastLuTable_,
                                       uint16_t usLuTableSize_);
static int16_t LoopUpNtcTemperatureC(uint16_t usAdcVal_, 
                                     const NtcLookUpTableTypeDef *pastLuTable_, 
                                     uint16_t usLuTableSize_);
static uint16_t GetVrefint_mV(void);

/*=============================================================================
 * Public Function Definitions
 *============================================================================*/
/**
 * @brief   Start thermal ADC1 sampling
 * 
 * @details ADC sampling triggered by TIM6 TRGO at 1KHz, DMA buffer is circular and holds the last 10 frames of ADC values.
 * 
 * @return  True if all tests passed
 */
bool StartThermalAdcSampling(void)
{
  // Init the DMA buffer to zero
  for (uint32_t i = 0U; i < THERMAL_ADC_FRAME_SIZE; i++)
    astTheAdc1RawVals[i] = (Adc1ValTypeDef){0U};

  stTheAdc1RawSnapshot = (Adc1ValTypeDef){0U};
  stTheAdc1FilterState = (Adc1FilterStateTypeDef){0U};

  osDelay(1);

  // Register the ADC conversion complete callback
  if (HAL_ADC_RegisterCallback(&hadc1,
                               HAL_ADC_CONVERSION_COMPLETE_CB_ID,
                               ThermalAdcConvCompleteCallback) != HAL_OK)
  {
    DPRINTF_ERROR(DBG_MASK_THERMAL, "ADC1 ISR REGISTER FAIL\r\n");
    return false;
  } 
  else
  {
    DPRINTF_DEBUG(DBG_MASK_THERMAL, "ADC1 ISR registered\r\n"); 
  }              

  // Configure and calibrate the ADC first
  if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK)
  {
    DPRINTF_ERROR(DBG_MASK_THERMAL, "ADC1 CALIBRATION FAIL\r\n");
    return false;    
  }
  else
  {
    DPRINTF_DEBUG(DBG_MASK_THERMAL, "ADC1 Calibrated\r\n");  
  }

  // Start ADC and DMA. ADC sampling waits for TIM6 TRGO.
  if (HAL_ADC_Start_DMA(&hadc1,
                        (uint32_t *)astTheAdc1RawVals,
                        (ADC1_CHANNEL_COUNT * THERMAL_ADC_FRAME_SIZE)) != HAL_OK)
  {
    DPRINTF_ERROR(DBG_MASK_THERMAL, "ADC1 DMA START FAIL\r\n");
    return false;    
  }
  else
  {
    DPRINTF_DEBUG(DBG_MASK_THERMAL, "ADC1 DMA Started\r\n");
  }
    
  // Start the timer6.
  if (HAL_TIM_Base_Start(&htim6) != HAL_OK)
  {
    DPRINTF_ERROR(DBG_MASK_THERMAL, "ADC1 TIM6 TROG START FAIL\r\n");
    return false;    
  }
  else 
  {
    DPRINTF_DEBUG(DBG_MASK_THERMAL, "ADC1 TIM6 TROG Started\r\n");
  }

  // Wait for the first averaged DMA frame to seed the filtered ADC values.
  osDelay(THERMAL_ADC_FRAME_SIZE + 1U);

  bool bFilterInitialized;
  Adc1ValTypeDef stAdc1Vals;

  __disable_irq();
  bFilterInitialized = stTheAdc1FilterState.bFilterInitialized;
  stAdc1Vals = stTheAdc1FilterState.stVals;
  __enable_irq();

  if (bFilterInitialized == false)
  {
    DPRINTF_ERROR(DBG_MASK_THERMAL, "ADC1 FILTER INIT FAIL\r\n");
    return false;
  }

  // Verify the NTC readings are within the expected range.
  if (!ThermalAdcVerifyNtcReadings(&stAdc1Vals))
    return false;

  DPRINTF_INFO(DBG_MASK_THERMAL, "Thermal ADC1 Sampling Started\r\n");

  return true;
}

/**
 * @brief Get Mcu junciton temperature in celsius
 */
int16_t GetMcuJunctionTemperatureC(void)
{
  // Avoid division by 0
  if (stTheAdc1FilterState.stVals.ulVrefint == 0U)
    return INT16_MIN;

  return __HAL_ADC_CALC_TEMPERATURE(GetVrefint_mV(), 
                                    stTheAdc1FilterState.stVals.ulMcuTemp, 
                                    __HAL_ADC_GET_RESOLUTION(&hadc1));
}

/**
 * @brief Get heater surface temperature in celsius
 */
int16_t GetHeaterNtcTemperatureC(void)
{
  // Handle open circuit and short circuit case (TODO)

  return LoopUpNtcTemperatureC(stTheAdc1FilterState.stVals.ulHeaterNtc, 
                               astSunLordNtcLookUpTable, 
                               ARRAY_SIZE(astSunLordNtcLookUpTable));
}

/**
 * @brief Get TEC1 surface temperature in celsius
 */
int16_t GetCooler1NtcTemperatureC(void)
{
  // Handle open circuit and short circuit case (TODO)

  return LoopUpNtcTemperatureC(stTheAdc1FilterState.stVals.ulCooler1Ntc, 
                               astVishayNtcLookUpTable, 
                               ARRAY_SIZE(astVishayNtcLookUpTable));
}

/**
 * @brief Get TEC2 surface temperature in celsius
 */
int16_t GetCooler2NtcTemperatureC(void)
{
  // Handle open circuit and short circuit case (TODO)

  return LoopUpNtcTemperatureC(stTheAdc1FilterState.stVals.ulCooler2Ntc, 
                               astVishayNtcLookUpTable, 
                               ARRAY_SIZE(astVishayNtcLookUpTable));
}

/**
 * @brief Get TEC1 heatsink surface temperature in celsius
 */
int16_t GetCooler1HeatSinkNtcTemperatureC(void)
{
  // Handle open circuit and short circuit case (TODO)

  return LoopUpNtcTemperatureC(stTheAdc1FilterState.stVals.ulCooler1HsNtc, 
                               astVishayNtcLookUpTable, 
                               ARRAY_SIZE(astVishayNtcLookUpTable));
}

/**
 * @brief Get TEC2 heatsink surface temperature in celsius
 */
int16_t GetCooler2HeatSinkNtcTemperatureC(void)
{
  // Handle open circuit and short circuit case (TODO)

  return LoopUpNtcTemperatureC(stTheAdc1FilterState.stVals.ulCooler2HsNtc, 
                               astVishayNtcLookUpTable, 
                               ARRAY_SIZE(astVishayNtcLookUpTable));
}

// Test
void PrintThermalAdcRawValues(void)
{
  Adc1ValTypeDef stAdc1RawVals;

  __disable_irq();
  stAdc1RawVals = stTheAdc1RawSnapshot;
  __enable_irq();

  DPRINTF_TRACE(DBG_MASK_THERMAL, "ADC1_RAW_CH7: %d, ADC1_RAW_CH8: %d, ADC1_RAW_CH9: %d, ADC1_RAW_CH1: %d, ADC1_RAW_CH2: %d, ADC1_RAW_MCUTEMP: %d, ADC1_RAW_VREFINT: %d\r\n",
                stAdc1RawVals.ulHeaterNtc, 
                stAdc1RawVals.ulCooler1Ntc, 
                stAdc1RawVals.ulCooler2Ntc, 
                stAdc1RawVals.ulCooler1HsNtc,
                stAdc1RawVals.ulCooler2HsNtc, 
                stAdc1RawVals.ulMcuTemp, 
                stAdc1RawVals.ulVrefint);
}

// Test
void PrintThermalAdcValues(void)
{
  Adc1ValTypeDef stAdc1Vals;

  __disable_irq();
  stAdc1Vals = stTheAdc1FilterState.stVals;
  __enable_irq();

  DPRINTF_TRACE(DBG_MASK_THERMAL, 
                "ADC1_CH7: %d, ADC1_CH8: %d, ADC1_CH9: %d, ADC1_CH1: %d, ADC1_CH2: %d, ADC1_MCUTEMP: %d, ADC_VREFINT: %d\r\n",
                stAdc1Vals.ulHeaterNtc, 
                stAdc1Vals.ulCooler1Ntc, 
                stAdc1Vals.ulCooler2Ntc, 
                stAdc1Vals.ulCooler1HsNtc,
                stAdc1Vals.ulCooler2HsNtc, 
                stAdc1Vals.ulMcuTemp, 
                stAdc1Vals.ulVrefint);
}

// Test
void PrintThermalAdcTemperatures(void)
{
  DPRINTF_TRACE(DBG_MASK_THERMAL, 
                "HEATER_NTC: %d C, COOLER1_NTC: %d C, COOLER2_NTC: %d C, COOLER1_HS_NTC: %d C, COOLER2_HS_NTC: %d C, MCU_TEMP: %d C\r\n",
                GetHeaterNtcTemperatureC(),
                GetCooler1NtcTemperatureC(),
                GetCooler2NtcTemperatureC(),
                GetCooler1HeatSinkNtcTemperatureC(),
                GetCooler2HeatSinkNtcTemperatureC(),
                GetMcuJunctionTemperatureC());
}

/*=============================================================================
 * Private Function Definitions
 *============================================================================*/
/**
 * @brief Average the DMA frame buffer and update the ADC1 IIR filter state.
 */
static inline void ThermalAdcUpdateFilteredValues(void)
{
  ADC1AccumValTypeDef stAdc1Sums = {0U};
  Adc1ValTypeDef stAdc1AvgVals;

  // Sum every channel across the completed DMA frame buffer.
  for (uint8_t i = 0U; i < THERMAL_ADC_FRAME_SIZE; i++)
  {
    stAdc1Sums.ulHeaterNtc += astTheAdc1RawVals[i].ulHeaterNtc;
    stAdc1Sums.ulCooler1Ntc += astTheAdc1RawVals[i].ulCooler1Ntc;
    stAdc1Sums.ulCooler2Ntc += astTheAdc1RawVals[i].ulCooler2Ntc;
    stAdc1Sums.ulCooler1HsNtc += astTheAdc1RawVals[i].ulCooler1HsNtc;
    stAdc1Sums.ulCooler2HsNtc += astTheAdc1RawVals[i].ulCooler2HsNtc;
    stAdc1Sums.ulMcuTemp += astTheAdc1RawVals[i].ulMcuTemp;
    stAdc1Sums.ulVrefint += astTheAdc1RawVals[i].ulVrefint;
  }

  // Compute the average for each channel by right shifting the sum by THERMAL_ADC_AVG_SHIFT.
  stAdc1AvgVals.ulHeaterNtc = (uint16_t)(stAdc1Sums.ulHeaterNtc >> THERMAL_ADC_AVG_SHIFT);
  stAdc1AvgVals.ulCooler1Ntc = (uint16_t)(stAdc1Sums.ulCooler1Ntc >> THERMAL_ADC_AVG_SHIFT);
  stAdc1AvgVals.ulCooler2Ntc = (uint16_t)(stAdc1Sums.ulCooler2Ntc >> THERMAL_ADC_AVG_SHIFT);
  stAdc1AvgVals.ulCooler1HsNtc = (uint16_t)(stAdc1Sums.ulCooler1HsNtc >> THERMAL_ADC_AVG_SHIFT);
  stAdc1AvgVals.ulCooler2HsNtc = (uint16_t)(stAdc1Sums.ulCooler2HsNtc >> THERMAL_ADC_AVG_SHIFT);
  stAdc1AvgVals.ulMcuTemp = (uint16_t)(stAdc1Sums.ulMcuTemp >> THERMAL_ADC_AVG_SHIFT);
  stAdc1AvgVals.ulVrefint = (uint16_t)(stAdc1Sums.ulVrefint >> THERMAL_ADC_AVG_SHIFT);

  // Seed the IIR accumulator from the first averaged sample to avoid startup ramp.
  if (stTheAdc1FilterState.bFilterInitialized == false)
  {
    // Left shift to enable fractional bits in the fixed-point IIR accumulator.
    stTheAdc1FilterState.stFilterAccVals.ulHeaterNtc = ((uint32_t)stAdc1AvgVals.ulHeaterNtc << THERMAL_ADC_Q_SHIFT);
    stTheAdc1FilterState.stFilterAccVals.ulCooler1Ntc = ((uint32_t)stAdc1AvgVals.ulCooler1Ntc << THERMAL_ADC_Q_SHIFT);
    stTheAdc1FilterState.stFilterAccVals.ulCooler2Ntc = ((uint32_t)stAdc1AvgVals.ulCooler2Ntc << THERMAL_ADC_Q_SHIFT);
    stTheAdc1FilterState.stFilterAccVals.ulCooler1HsNtc = ((uint32_t)stAdc1AvgVals.ulCooler1HsNtc << THERMAL_ADC_Q_SHIFT);
    stTheAdc1FilterState.stFilterAccVals.ulCooler2HsNtc = ((uint32_t)stAdc1AvgVals.ulCooler2HsNtc << THERMAL_ADC_Q_SHIFT);
    stTheAdc1FilterState.stFilterAccVals.ulMcuTemp = ((uint32_t)stAdc1AvgVals.ulMcuTemp << THERMAL_ADC_Q_SHIFT);
    stTheAdc1FilterState.stFilterAccVals.ulVrefint = ((uint32_t)stAdc1AvgVals.ulVrefint << THERMAL_ADC_Q_SHIFT);
    stTheAdc1FilterState.bFilterInitialized = true;
  }

  // Apply one independent IIR filter per ADC channel.
  stTheAdc1FilterState.stVals.ulHeaterNtc = Get1stOrderIIRFilteredVal(stAdc1AvgVals.ulHeaterNtc, 
                                                                      &stTheAdc1FilterState.stFilterAccVals.ulHeaterNtc);
  stTheAdc1FilterState.stVals.ulCooler1Ntc = Get1stOrderIIRFilteredVal(stAdc1AvgVals.ulCooler1Ntc, 
                                                                       &stTheAdc1FilterState.stFilterAccVals.ulCooler1Ntc);
  stTheAdc1FilterState.stVals.ulCooler2Ntc = Get1stOrderIIRFilteredVal(stAdc1AvgVals.ulCooler2Ntc, 
                                                                       &stTheAdc1FilterState.stFilterAccVals.ulCooler2Ntc);
  stTheAdc1FilterState.stVals.ulCooler1HsNtc = Get1stOrderIIRFilteredVal(stAdc1AvgVals.ulCooler1HsNtc, 
                                                                         &stTheAdc1FilterState.stFilterAccVals.ulCooler1HsNtc);
  stTheAdc1FilterState.stVals.ulCooler2HsNtc = Get1stOrderIIRFilteredVal(stAdc1AvgVals.ulCooler2HsNtc, 
                                                                         &stTheAdc1FilterState.stFilterAccVals.ulCooler2HsNtc);
  stTheAdc1FilterState.stVals.ulMcuTemp = Get1stOrderIIRFilteredVal(stAdc1AvgVals.ulMcuTemp, 
                                                                    &stTheAdc1FilterState.stFilterAccVals.ulMcuTemp);
  stTheAdc1FilterState.stVals.ulVrefint = Get1stOrderIIRFilteredVal(stAdc1AvgVals.ulVrefint, 
                                                                    &stTheAdc1FilterState.stFilterAccVals.ulVrefint);
}

/**
 * @brief         Apply one fixed-point first-order IIR filter update.
 * 
 * @details       The IIR filter is implemented in fixed-point Q format
 *                Formula: y[n] = (1 - alpha) * y[n-1] + alpha * x[n] 
 *                         y[n] = y[n-1] + alpha * (x[n] - y[n-1])
 *
 * @param[in]     usVal_     Averaged ADC sample
 * @param[in,out] pulAccVal_ Fixed-point accumulator for one ADC channel
 *
 * @return        Filtered ADC sample rounded back to integer ADC counts
 */
static inline uint16_t Get1stOrderIIRFilteredVal(uint16_t usVal_, 
                                                 uint32_t *pulAccVal_)
{
  ASSERT(pulAccVal_ != NULL);

  // Enable fractional bits
  uint32_t ulAvgValQ = ((uint32_t)usVal_ << THERMAL_ADC_Q_SHIFT);

  // Move the accumulator 1 / 2^THERMAL_ADC_IIR_ALPHA_SHIFT toward the new average.
  if (ulAvgValQ >= *pulAccVal_)
    *pulAccVal_ += ((ulAvgValQ - *pulAccVal_) >> THERMAL_ADC_IIR_ALPHA_SHIFT);
  else
    *pulAccVal_ -= ((*pulAccVal_ - ulAvgValQ) >> THERMAL_ADC_IIR_ALPHA_SHIFT);

  // Convert from fixed-point accumulator back to rounded integer ADC counts.
  return (uint16_t)((*pulAccVal_ + (1UL << (THERMAL_ADC_Q_SHIFT - 1U))) >> THERMAL_ADC_Q_SHIFT);
}

/**
 * @brief     Check filtered NTC ADC readings for open and short-circuit faults.
 *
 * @param[in] pstAdc1Vals_ Filtered ADC1 values to check
 *
 * @return    true if all NTC readings are valid, false otherwise
 */
static bool ThermalAdcVerifyNtcReadings(const Adc1ValTypeDef *pstAdc1Vals_)
{
  ASSERT(pstAdc1Vals_ != NULL);

  bool bAnyFailed = true;

  bAnyFailed &= ThermalAdcVerifyNtcReading("HEATER_NTC",
                                           pstAdc1Vals_->ulHeaterNtc,
                                           astSunLordNtcLookUpTable,
                                           ARRAY_SIZE(astSunLordNtcLookUpTable));
  bAnyFailed &= ThermalAdcVerifyNtcReading("COOLER1_NTC",
                                           pstAdc1Vals_->ulCooler1Ntc,
                                           astVishayNtcLookUpTable,
                                           ARRAY_SIZE(astVishayNtcLookUpTable));
  bAnyFailed &= ThermalAdcVerifyNtcReading("COOLER2_NTC",
                                           pstAdc1Vals_->ulCooler2Ntc,
                                           astVishayNtcLookUpTable,
                                           ARRAY_SIZE(astVishayNtcLookUpTable));
  bAnyFailed &= ThermalAdcVerifyNtcReading("COOLER1_HS_NTC",
                                           pstAdc1Vals_->ulCooler1HsNtc,
                                           astVishayNtcLookUpTable,
                                           ARRAY_SIZE(astVishayNtcLookUpTable));
  bAnyFailed &= ThermalAdcVerifyNtcReading("COOLER2_HS_NTC",
                                           pstAdc1Vals_->ulCooler2HsNtc,
                                           astVishayNtcLookUpTable,
                                           ARRAY_SIZE(astVishayNtcLookUpTable));

  return bAnyFailed;
}

/**
 * @brief     Print an NTC wiring fault when the filtered ADC value is out of range.
 *
 * @param[in] pcNtcName_     NTC channel name
 * @param[in] usAdcVal_      Filtered ADC reading
 * @param[in] pastLuTable_   Pointer to the NTC lookup table
 * @param[in] usLuTableSize_ Size of the NTC lookup table
 *
 * @return    true if the NTC reading is valid, false otherwise
 */
static bool ThermalAdcVerifyNtcReading(const char *pcNtcName_, 
                                       uint16_t usAdcVal_,
                                       const NtcLookUpTableTypeDef *pastLuTable_,
                                       uint16_t usLuTableSize_)
{
  ASSERT(pcNtcName_ != NULL);
  ASSERT(pastLuTable_ != NULL);
  ASSERT(usLuTableSize_ >= 2U);

  if (usAdcVal_ > NTC_ADC_OPEN_CIRCUIT_THRESH)
  {
    DPRINTF_ERROR(DBG_MASK_THERMAL, "%s ADC NC\r\n", pcNtcName_);
    return false;
  }
  else if (usAdcVal_ < NTC_ADC_GND_SHORT_THRESH)
  {
    DPRINTF_ERROR(DBG_MASK_THERMAL, "%s ADC GROUNDED\r\n", pcNtcName_);
    return false;
  }
  else if (usAdcVal_ >= (pastLuTable_[0].usAdcVal + ADC1_INVIL_MARGIN))
  {
    DPRINTF_ERROR(DBG_MASK_THERMAL, "%s ADC OVER RANGE\r\n", pcNtcName_);
    return false;
  }
  else if (usAdcVal_ <= (pastLuTable_[usLuTableSize_ - 1U].usAdcVal - ADC1_INVIL_MARGIN))
  {
    DPRINTF_ERROR(DBG_MASK_THERMAL, "%s ADC UNDER RANGE\r\n", pcNtcName_);
    return false;
  }
  else
  {
    DPRINTF_DEBUG(DBG_MASK_THERMAL, "%s reading verified: %d\r\n", pcNtcName_, usAdcVal_);
    return true;
  }
}

/**
 * @brief     Convert an NTC ADC value to temperature using a lookup table.
 * 
 * @details   The function performs a binary search on the provided lookup table to find the two entries 
 *            that bracket the given ADC value. It then performs linear interpolation between these two entries 
 *            to estimate the temperature corresponding to the ADC value. 
 * 
 * @param[in] usAdcVal_       The ADC value to be converted to temperature.
 * @param[in] pastLuTable_    Pointer to the lookup table containing ADC values and corresponding temperatures.
 * @param[in] usLuTableSize_  The size of the lookup table.
 * 
 * @attention If the ADC value is outside the range of the lookup table, 
 *            it returns INT16_MAX or INT16_MIN to indicate that the temperature is out of range.
 */
static int16_t LoopUpNtcTemperatureC(uint16_t usAdcVal_, 
                                     const NtcLookUpTableTypeDef *pastLuTable_, 
                                     uint16_t usLuTableSize_)
{
  ASSERT(pastLuTable_ != NULL);
  ASSERT(usLuTableSize_ >= 2U);

  // If the ADC value is outside the table range, return the max values. 
  if (usAdcVal_ >= (pastLuTable_[0].usAdcVal + ADC1_INVIL_MARGIN))
    return INT16_MAX;
  else if (usAdcVal_ >= pastLuTable_[0].usAdcVal)
    return pastLuTable_[0].sTemperatureC;
  else if (usAdcVal_ <= (pastLuTable_[usLuTableSize_ - 1U].usAdcVal - ADC1_INVIL_MARGIN))
    return INT16_MIN;
  else if (usAdcVal_ <= pastLuTable_[usLuTableSize_ - 1U].usAdcVal)
    return pastLuTable_[usLuTableSize_ - 1U].sTemperatureC;

  // Binary search for the two table entries that bracket the ADC value.
  uint16_t usLow = 0U;
  uint16_t usHigh = usLuTableSize_ - 1U;
  while ((usHigh - usLow) > 1U)
  {
    // Divided by 2 
    uint16_t usMid = (usLow + usHigh) >> 1U;

    // Compare the ADC value at the mid index with the input ADC value to narrow down the search range.
    if (pastLuTable_[usMid].usAdcVal > usAdcVal_)
      usLow = usMid;
    else
      usHigh = usMid;
  }

  // Linear interpolation between the two bracketing table entries.
  uint16_t usAdcDiff = pastLuTable_[usLow].usAdcVal - pastLuTable_[usHigh].usAdcVal;
  int32_t lTempDiff = (int32_t)pastLuTable_[usHigh].sTemperatureC - (int32_t)pastLuTable_[usLow].sTemperatureC;
  uint16_t usAdcOffset = pastLuTable_[usLow].usAdcVal - usAdcVal_;

  // If the ADC difference is zero, return the lower temperature directly to avoid division by zero.
  if (usAdcDiff == 0U)
    return pastLuTable_[usLow].sTemperatureC;

  return (int16_t)((int32_t)pastLuTable_[usLow].sTemperatureC +
                   (((int32_t)usAdcOffset * lTempDiff + (int32_t)(usAdcDiff >> 1U)) / (int32_t)usAdcDiff));
}

/**
 * @brief Function for getting internal reference voltage in millivolt
 */
static uint16_t GetVrefint_mV(void)
{
  // Avoid division by 0
  if (stTheAdc1FilterState.stVals.ulVrefint == 0U)
    return 0U;

  return __HAL_ADC_CALC_VREFANALOG_VOLTAGE(stTheAdc1FilterState.stVals.ulVrefint,
                                           __HAL_ADC_GET_RESOLUTION(&hadc1));
}

/**
 * @brief Callback function for ADC conversion complete event
 * 
 * @details This function is called when the ADC DMA buffer is filled.
 */
static void ThermalAdcConvCompleteCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc == &hadc1)
  {
    stTheAdc1RawSnapshot = astTheAdc1RawVals[THERMAL_ADC_FRAME_SIZE - 1U];
    ThermalAdcUpdateFilteredValues();
  }
}
