/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

#include "stm32g4xx_nucleo.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define RCC_OSC32_IN_Pin GPIO_PIN_14
#define RCC_OSC32_IN_GPIO_Port GPIOC
#define RCC_OSC32_OUT_Pin GPIO_PIN_15
#define RCC_OSC32_OUT_GPIO_Port GPIOC
#define RCC_OSC_IN_Pin GPIO_PIN_0
#define RCC_OSC_IN_GPIO_Port GPIOF
#define RCC_OSC_OUT_Pin GPIO_PIN_1
#define RCC_OSC_OUT_GPIO_Port GPIOF
#define FAULT_LED_Pin GPIO_PIN_0
#define FAULT_LED_GPIO_Port GPIOC
#define HEATER_NTC_IN_Pin GPIO_PIN_1
#define HEATER_NTC_IN_GPIO_Port GPIOC
#define COOLER1_NTC_IN_Pin GPIO_PIN_2
#define COOLER1_NTC_IN_GPIO_Port GPIOC
#define COOLER2_NTC_IN_Pin GPIO_PIN_3
#define COOLER2_NTC_IN_GPIO_Port GPIOC
#define COOLER1HS_NTC_IN_Pin GPIO_PIN_0
#define COOLER1HS_NTC_IN_GPIO_Port GPIOA
#define COOLER2HS_NTC_IN_Pin GPIO_PIN_1
#define COOLER2HS_NTC_IN_GPIO_Port GPIOA
#define DBG_TX_Pin GPIO_PIN_2
#define DBG_TX_GPIO_Port GPIOA
#define DBG_RX_Pin GPIO_PIN_3
#define DBG_RX_GPIO_Port GPIOA
#define TRAYSTM_DIR_Pin GPIO_PIN_6
#define TRAYSTM_DIR_GPIO_Port GPIOA
#define TRAYSTM_EN_OUT_Pin GPIO_PIN_7
#define TRAYSTM_EN_OUT_GPIO_Port GPIOA
#define LIFTSTM_DIR_OUT_Pin GPIO_PIN_4
#define LIFTSTM_DIR_OUT_GPIO_Port GPIOC
#define LIFTSTM_PWM_OUT_Pin GPIO_PIN_0
#define LIFTSTM_PWM_OUT_GPIO_Port GPIOB
#define LIFTSTM_HOME_EXTI1_Pin GPIO_PIN_1
#define LIFTSTM_HOME_EXTI1_GPIO_Port GPIOB
#define LIFTSTM_HOME_EXTI1_EXTI_IRQn EXTI1_IRQn
#define DOORSM_PWM_OUT_Pin GPIO_PIN_2
#define DOORSM_PWM_OUT_GPIO_Port GPIOB
#define COOLER2HSM_PWM_OUT_Pin GPIO_PIN_10
#define COOLER2HSM_PWM_OUT_GPIO_Port GPIOB
#define COOLER1HSM_PWM_OUT_Pin GPIO_PIN_11
#define COOLER1HSM_PWM_OUT_GPIO_Port GPIOB
#define COOLER2_IRT_SCL_Pin GPIO_PIN_6
#define COOLER2_IRT_SCL_GPIO_Port GPIOC
#define COOLER2_IRT_SDA_Pin GPIO_PIN_7
#define COOLER2_IRT_SDA_GPIO_Port GPIOC
#define COOLER1_IRT_SCL_Pin GPIO_PIN_8
#define COOLER1_IRT_SCL_GPIO_Port GPIOC
#define COOLER1_IRT_SDA_Pin GPIO_PIN_9
#define COOLER1_IRT_SDA_GPIO_Port GPIOC
#define LIFTSTM_EN_OUT_Pin GPIO_PIN_8
#define LIFTSTM_EN_OUT_GPIO_Port GPIOA
#define COOLER2_PWM_OUT_Pin GPIO_PIN_9
#define COOLER2_PWM_OUT_GPIO_Port GPIOA
#define COOLER1_PWM_OUT_Pin GPIO_PIN_10
#define COOLER1_PWM_OUT_GPIO_Port GPIOA
#define HEATER_PWM_OUT_Pin GPIO_PIN_11
#define HEATER_PWM_OUT_GPIO_Port GPIOA
#define TRAYSTM_HOME_EXTI12_Pin GPIO_PIN_12
#define TRAYSTM_HOME_EXTI12_GPIO_Port GPIOA
#define TRAYSTM_HOME_EXTI12_EXTI_IRQn EXTI15_10_IRQn
#define T_SWDIO_Pin GPIO_PIN_13
#define T_SWDIO_GPIO_Port GPIOA
#define T_SWCLK_Pin GPIO_PIN_14
#define T_SWCLK_GPIO_Port GPIOA
#define TRAYSTM_PWM_OUT_Pin GPIO_PIN_15
#define TRAYSTM_PWM_OUT_GPIO_Port GPIOA
#define RPI_TX_Pin GPIO_PIN_10
#define RPI_TX_GPIO_Port GPIOC
#define RPI_RX_Pin GPIO_PIN_11
#define RPI_RX_GPIO_Port GPIOC
#define TRAYSTM_TX_RX_Pin GPIO_PIN_12
#define TRAYSTM_TX_RX_GPIO_Port GPIOC
#define TRAYSTM_DIAG_EXTI2_Pin GPIO_PIN_2
#define TRAYSTM_DIAG_EXTI2_GPIO_Port GPIOD
#define TRAYSTM_DIAG_EXTI2_EXTI_IRQn EXTI2_IRQn
#define T_SWO_Pin GPIO_PIN_3
#define T_SWO_GPIO_Port GPIOB
#define HEATER_IRT_SDA_Pin GPIO_PIN_7
#define HEATER_IRT_SDA_GPIO_Port GPIOB
#define HEATER_IRT_SCL_Pin GPIO_PIN_8
#define HEATER_IRT_SCL_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
