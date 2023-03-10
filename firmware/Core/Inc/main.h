/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
#include "stm32f4xx_hal.h"

#include "stm32f4xx_ll_dma.h"
#include "stm32f4xx_ll_spi.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_cortex.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_system.h"
#include "stm32f4xx_ll_utils.h"
#include "stm32f4xx_ll_pwr.h"
#include "stm32f4xx_ll_gpio.h"

#include "stm32f4xx_ll_exti.h"

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
#define BUTTON_ROW0_Pin GPIO_PIN_13
#define BUTTON_ROW0_GPIO_Port GPIOC
#define BUTTON_ROW1_Pin GPIO_PIN_14
#define BUTTON_ROW1_GPIO_Port GPIOC
#define BUTTON_ROW2_Pin GPIO_PIN_15
#define BUTTON_ROW2_GPIO_Port GPIOC
#define POWER_OFF_Pin GPIO_PIN_0
#define POWER_OFF_GPIO_Port GPIOC
#define LCD_CE_Pin GPIO_PIN_1
#define LCD_CE_GPIO_Port GPIOC
#define LCD_DC_Pin GPIO_PIN_2
#define LCD_DC_GPIO_Port GPIOC
#define NRF_INT_Pin GPIO_PIN_3
#define NRF_INT_GPIO_Port GPIOC
#define FLASH_nCS_Pin GPIO_PIN_4
#define FLASH_nCS_GPIO_Port GPIOA
#define NRF_CE_Pin GPIO_PIN_4
#define NRF_CE_GPIO_Port GPIOC
#define NRF_nCS_Pin GPIO_PIN_5
#define NRF_nCS_GPIO_Port GPIOC
#define VOLTAGE_MONITOR_BT_Pin GPIO_PIN_0
#define VOLTAGE_MONITOR_BT_GPIO_Port GPIOB
#define VOLTAGE_MONITOR_5_Pin GPIO_PIN_1
#define VOLTAGE_MONITOR_5_GPIO_Port GPIOB
#define AMP_SHUTDOWN_Pin GPIO_PIN_2
#define AMP_SHUTDOWN_GPIO_Port GPIOB
#define VIBRATION_Pin GPIO_PIN_10
#define VIBRATION_GPIO_Port GPIOB
#define LCD_BACKLIGHT_Pin GPIO_PIN_7
#define LCD_BACKLIGHT_GPIO_Port GPIOC
#define SD_CD_Pin GPIO_PIN_4
#define SD_CD_GPIO_Port GPIOB
#define BUTTON_COL2_Pin GPIO_PIN_5
#define BUTTON_COL2_GPIO_Port GPIOB
#define BUTTON_COL1_Pin GPIO_PIN_8
#define BUTTON_COL1_GPIO_Port GPIOB
#define BUTTON_COL0_Pin GPIO_PIN_9
#define BUTTON_COL0_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
