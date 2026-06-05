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
#include "stm32h7xx_hal.h"

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
#define SENSOR_02_MOTOR_RPM_Pin GPIO_PIN_0
#define SENSOR_02_MOTOR_RPM_GPIO_Port GPIOA
#define SENSOR_03_BATTERY_VOLTAGE_Pin GPIO_PIN_6
#define SENSOR_03_BATTERY_VOLTAGE_GPIO_Port GPIOA
#define LD1_Pin GPIO_PIN_0
#define LD1_GPIO_Port GPIOB
#define SENSOR_04_FUEL_LEVEL_Pin GPIO_PIN_1
#define SENSOR_04_FUEL_LEVEL_GPIO_Port GPIOB
#define SENSOR_01_COOLANT_TEMPERATURE_Pin GPIO_PIN_11
#define SENSOR_01_COOLANT_TEMPERATURE_GPIO_Port GPIOF
#define SENSOR_05_TURN_SIGNAL_Pin GPIO_PIN_0
#define SENSOR_05_TURN_SIGNAL_GPIO_Port GPIOD
#define SENSOR_06_HIGH_BEAM_Pin GPIO_PIN_1
#define SENSOR_06_HIGH_BEAM_GPIO_Port GPIOD
#define SENSOR_07_OIL_PRESSURE_0_3_BAR_Pin GPIO_PIN_2
#define SENSOR_07_OIL_PRESSURE_0_3_BAR_GPIO_Port GPIOD
#define SENSOR_08_OIL_PRESSURE_1_8_BAR_Pin GPIO_PIN_3
#define SENSOR_08_OIL_PRESSURE_1_8_BAR_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
