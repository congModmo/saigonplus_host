/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
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
#include "stm32f0xx_hal.h"

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
#define FLASH_HOLD_Pin GPIO_PIN_0
#define FLASH_HOLD_GPIO_Port GPIOC
#define GSM_LS_Pin GPIO_PIN_2
#define GSM_LS_GPIO_Port GPIOC
#define GSM_POWER_Pin GPIO_PIN_3
#define GSM_POWER_GPIO_Port GPIOC
#define FLASH_WP_Pin GPIO_PIN_0
#define FLASH_WP_GPIO_Port GPIOA
#define BLE_RESET_Pin GPIO_PIN_1
#define BLE_RESET_GPIO_Port GPIOA
#define BLE_TX_Pin GPIO_PIN_2
#define BLE_TX_GPIO_Port GPIOA
#define BLE_RX_Pin GPIO_PIN_3
#define BLE_RX_GPIO_Port GPIOA
#define FLASH_CS_Pin GPIO_PIN_4
#define FLASH_CS_GPIO_Port GPIOA
#define LTE_TX_Pin GPIO_PIN_4
#define LTE_TX_GPIO_Port GPIOC
#define LTE_RX_Pin GPIO_PIN_5
#define LTE_RX_GPIO_Port GPIOC
#define BUZZER_Pin GPIO_PIN_0
#define BUZZER_GPIO_Port GPIOB
#define GSM_RTS_Pin GPIO_PIN_1
#define GSM_RTS_GPIO_Port GPIOB
#define GSM_CTS_Pin GPIO_PIN_2
#define GSM_CTS_GPIO_Port GPIOB
#define GPS_RESET_Pin GPIO_PIN_12
#define GPS_RESET_GPIO_Port GPIOB
#define GPS_BOOT_Pin GPIO_PIN_14
#define GPS_BOOT_GPIO_Port GPIOB
#define GSM_RESET_Pin GPIO_PIN_15
#define GSM_RESET_GPIO_Port GPIOB
#define LED_FRONT_Pin GPIO_PIN_6
#define LED_FRONT_GPIO_Port GPIOC
#define LED_BLUE_Pin GPIO_PIN_7
#define LED_BLUE_GPIO_Port GPIOC
#define LED_GREEN_Pin GPIO_PIN_8
#define LED_GREEN_GPIO_Port GPIOC
#define DISPLAY_TX_Pin GPIO_PIN_9
#define DISPLAY_TX_GPIO_Port GPIOA
#define DISPLAY_RX_Pin GPIO_PIN_10
#define DISPLAY_RX_GPIO_Port GPIOA
#define LED_RED_Pin GPIO_PIN_11
#define LED_RED_GPIO_Port GPIOA
#define LTE_DCD_Pin GPIO_PIN_12
#define LTE_DCD_GPIO_Port GPIOA
#define GPS_TX_Pin GPIO_PIN_10
#define GPS_TX_GPIO_Port GPIOC
#define GPS_RX_Pin GPIO_PIN_11
#define GPS_RX_GPIO_Port GPIOC
#define DEBUG_TX_Pin GPIO_PIN_12
#define DEBUG_TX_GPIO_Port GPIOC
#define DEBUG_RX_Pin GPIO_PIN_2
#define DEBUG_RX_GPIO_Port GPIOD
#define LOCK_Pin GPIO_PIN_3
#define LOCK_GPIO_Port GPIOB
#define GSM_DSR_Pin GPIO_PIN_5
#define GSM_DSR_GPIO_Port GPIOB
#define GSM_DTR_Pin GPIO_PIN_6
#define GSM_DTR_GPIO_Port GPIOB
#define IMU_INT_Pin GPIO_PIN_7
#define IMU_INT_GPIO_Port GPIOB
#define IMU_SCL_Pin GPIO_PIN_8
#define IMU_SCL_GPIO_Port GPIOB
#define IMU_SDA_Pin GPIO_PIN_9
#define IMU_SDA_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
