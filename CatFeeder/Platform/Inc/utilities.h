/******************************************************************************
 * @file    utilities.h
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

#ifndef INC_UTILITIES_H_
#define INC_UTILITIES_H_

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * Common Math
 *===========================================================================*/
#ifndef MIN
#define MIN(a_, b_)          (((a_) < (b_)) ? (a_) : (b_))
#endif

#ifndef MAX
#define MAX(a_, b_)          (((a_) > (b_)) ? (a_) : (b_))
#endif

#ifndef ABS
#define ABS(x_)              (((x_) < 0) ? -(x_) : (x_))
#endif

#ifndef CLAMP
#define CLAMP(x_, lo_, hi_)  (((x_) < (lo_)) ? (lo_) : (((x_) > (hi_)) ? (hi_) : (x_)))
#endif 

#ifndef IS_EVEN
#define IS_EVEN(x_)          ((((x_) & 1U) == 0U))
#endif

#ifndef IS_ODD
#define IS_ODD(x_)           ((((x_) & 1U) != 0U))
#endif

#ifndef IS_POWER_OF_TWO
#define IS_POWER_OF_TWO(x_)  (((x_) != 0U) && (((x_) & ((x_) - 1U)) == 0U))
#endif
/*=============================================================================
 * Array
 *===========================================================================*/
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr_)  (sizeof(arr_) / sizeof((arr_)[0]))
#endif

/*=============================================================================
 * Alignment
 *===========================================================================*/
#define ALIGN_UP(val_, align_)     (((val_) + ((align_) - 1U)) & ~((align_) - 1U))
#define ALIGN_DOWN(val_, align_)   ((val_) & ~((align_) - 1U))

/*=============================================================================
 * Unit Conversion
 *===========================================================================*/
#define KHZ(x_)              ((x_) * 1000UL)
#define MHZ(x_)              ((x_) * 1000000UL)

#define MS_TO_US(ms_)        ((ms_) * 1000UL)
#define US_TO_MS(us_)        ((us_) / 1000UL)

#define C_TO_K(tempC_)      ((tempC_) + 273.15f)
#define K_TO_C(tempK_)      ((tempK_) - 273.15f)

#define C_TO_F(tempC_)      (((tempC_) * 9.0f / 5.0f) + 32.0f)
#define F_TO_C(tempF_)      (((tempF_) - 32.0f) * 5.0f / 9.0f)

#define K_TO_F(tempK_)      C_TO_F(K_TO_C(tempK_))
#define F_TO_K(tempF_)      C_TO_K(F_TO_C(tempF_))

/*=============================================================================
 * Misc
 *===========================================================================*/
#ifndef UNUSED
#define UNUSED(x_)           ((void)(x_))
#endif

#ifdef __cplusplus
}
#endif

#endif /* INC_UTILITIES_H_ */
