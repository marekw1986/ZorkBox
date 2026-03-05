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
#include "stm32f4xx_hal.h"

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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SDIO_CD_Pin GPIO_PIN_9
#define SDIO_CD_GPIO_Port GPIOC
#define PS2_DATA_Pin GPIO_PIN_11
#define PS2_DATA_GPIO_Port GPIOA
#define PS2_CLK_Pin GPIO_PIN_12
#define PS2_CLK_GPIO_Port GPIOA
#define HSYNC_Pin GPIO_PIN_15
#define HSYNC_GPIO_Port GPIOA
#define RED0_Pin GPIO_PIN_3
#define RED0_GPIO_Port GPIOB
#define RED1_Pin GPIO_PIN_4
#define RED1_GPIO_Port GPIOB
#define GREEN0_Pin GPIO_PIN_5
#define GREEN0_GPIO_Port GPIOB
#define GREEN1_Pin GPIO_PIN_6
#define GREEN1_GPIO_Port GPIOB
#define BLUE0_Pin GPIO_PIN_7
#define BLUE0_GPIO_Port GPIOB
#define BLUE1_Pin GPIO_PIN_8
#define BLUE1_GPIO_Port GPIOB
#define VSYNC_Pin GPIO_PIN_9
#define VSYNC_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
