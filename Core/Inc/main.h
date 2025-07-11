/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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

#include "stm32g0xx_ll_dma.h"
#include "stm32g0xx_ll_rcc.h"
#include "stm32g0xx_ll_bus.h"
#include "stm32g0xx_ll_system.h"
#include "stm32g0xx_ll_exti.h"
#include "stm32g0xx_ll_cortex.h"
#include "stm32g0xx_ll_utils.h"
#include "stm32g0xx_ll_pwr.h"
#include "stm32g0xx_ll_spi.h"
#include "stm32g0xx_ll_usart.h"
#include "stm32g0xx_ll_gpio.h"

#if defined(USE_FULL_ASSERT)
#include "stm32_assert.h"
#endif /* USE_FULL_ASSERT */

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "app.h"

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
#define OE_Pin LL_GPIO_PIN_7
#define OE_GPIO_Port GPIOA
#define LAT_Pin LL_GPIO_PIN_1
#define LAT_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

#define OE_H    (OE_GPIO_Port->BSRR=OE_Pin)
#define OE_L    (OE_GPIO_Port->BRR=OE_Pin)
#define LAT_H    (LAT_GPIO_Port->BSRR=LAT_Pin)
#define LAT_L    (LAT_GPIO_Port->BRR=LAT_Pin)
#define CLK_H    (CLK_GPIO_Port->BSRR=CLK_Pin)
#define CLK_L    (CLK_GPIO_Port->BRR=CLK_Pin)
#define DAT_H    (DAT_GPIO_Port->BSRR=DAT_Pin)
#define DAT_L    (DAT_GPIO_Port->BRR=DAT_Pin)

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
