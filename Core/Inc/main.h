/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
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
#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define OSC32_IN_Pin GPIO_PIN_14
#define OSC32_IN_GPIO_Port GPIOC
#define OSC32_OUT_Pin GPIO_PIN_15
#define OSC32_OUT_GPIO_Port GPIOC
#define USB_FS_PWR_EN_Pin GPIO_PIN_10
#define USB_FS_PWR_EN_GPIO_Port GPIOF
#define PH0_MCU_Pin GPIO_PIN_0
#define PH0_MCU_GPIO_Port GPIOH
#define PH1_MCU_Pin GPIO_PIN_1
#define PH1_MCU_GPIO_Port GPIOH
#define TIM3_CH1_LINE_ST_Pin GPIO_PIN_6
#define TIM3_CH1_LINE_ST_GPIO_Port GPIOA
#define LD1_Pin GPIO_PIN_0
#define LD1_GPIO_Port GPIOB
#define COMP1_INM_Pin GPIO_PIN_1
#define COMP1_INM_GPIO_Port GPIOB
#define COMP1_INP_LINE1_VIDEO_Pin GPIO_PIN_2
#define COMP1_INP_LINE1_VIDEO_GPIO_Port GPIOB
#define COMP2_INP_CONTEINER_Pin GPIO_PIN_11
#define COMP2_INP_CONTEINER_GPIO_Port GPIOE
#define LD3_Pin GPIO_PIN_14
#define LD3_GPIO_Port GPIOB
#define STLINK_RX_Pin GPIO_PIN_8
#define STLINK_RX_GPIO_Port GPIOD
#define STLINK_TX_Pin GPIO_PIN_9
#define STLINK_TX_GPIO_Port GPIOD
#define USB_FS_OVCR_Pin GPIO_PIN_7
#define USB_FS_OVCR_GPIO_Port GPIOG
#define USB_FS_VBUS_Pin GPIO_PIN_9
#define USB_FS_VBUS_GPIO_Port GPIOA
#define USB_FS_ID_Pin GPIO_PIN_10
#define USB_FS_ID_GPIO_Port GPIOA
#define USB_FS_N_Pin GPIO_PIN_11
#define USB_FS_N_GPIO_Port GPIOA
#define USB_FS_P_Pin GPIO_PIN_12
#define USB_FS_P_GPIO_Port GPIOA
#define TIM17_CH1_LINE_CLK_Pin GPIO_PIN_9
#define TIM17_CH1_LINE_CLK_GPIO_Port GPIOB
#define LD2_Pin GPIO_PIN_1
#define LD2_GPIO_Port GPIOE
/* USER CODE BEGIN Private defines */

#define TIM3_CH2_LIGHT_Pin GPIO_PIN_7
#define TIM3_CH2_LIGHT_GPIO_Port GPIOA

#define TIM3_CH2_LIGHT_BLUE_Pin GPIO_PIN_5
#define TIM3_CH2_LIGHT_BLUE_GPIO_Port GPIOB

#define TIM3_CH1_LINE_ST_Pin GPIO_PIN_6
#define TIM3_CH1_LINE_ST_GPIO_Port GPIOA

#define S1_Pin GPIO_PIN_9
#define S1_GPIO_Port GPIOF

#define U1_TX_Pin GPIO_PIN_6
#define U1_TX_GPIO_Port GPIOB
#define U1_RX_Pin GPIO_PIN_7
#define U1_RX_GPIO_Port GPIOB

#define CONTAINER_DETECT_Pin GPIO_PIN_15
#define CONTAINER_DETECT_GPIO_Port GPIOD

#define OVER_COUNT_Pin GPIO_PIN_12
#define OVER_COUNT_GPIO_Port GPIOB

typedef struct
{
	uint16_t area;
	uint8_t cont;
	uint8_t sl;
} line_object_t;

#define NUM_PICES_PERIOD 8 // must equal power 2,  = 8, 16, 32, 64 ...
#define MIN_PICE_PERIOD 30

#define TRANSPARENT_OBJECT_OVERTIME 80
#define TRANSPARENT_OBJECT_CURRENT_LINE_OVERTIME 20

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
