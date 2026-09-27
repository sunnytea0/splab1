/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file         stm32c0xx_hal_msp.c
  * @brief        This file provides code for the MSP Initialization
  *               and de-Initialization codes.
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */


/**
  * Initializes the Global MSP.
  */
void HAL_MspInit(void)
{
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();
}


/**
  * @brief ADC MSP Initialization
  * @param hadc ADC handle pointer
  * @retval None
  */
void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  if (hadc->Instance == ADC1)
  {
    /* ADC clock configuration */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection = RCC_ADCCLKSOURCE_SYSCLK;

    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* ADC peripheral clock enable */
    __HAL_RCC_ADC_CLK_ENABLE();

    /* GPIOA clock enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /*
     * ADC1 GPIO Configuration
     *
     * PA0 ------> ADC1_IN0
     */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
}


/**
  * @brief ADC MSP De-Initialization
  * @param hadc ADC handle pointer
  * @retval None
  */
void HAL_ADC_MspDeInit(ADC_HandleTypeDef* hadc)
{
  if (hadc->Instance == ADC1)
  {
    __HAL_RCC_ADC_CLK_DISABLE();

    /*
     * ADC1 GPIO Configuration
     *
     * PA0 ------> ADC1_IN0
     */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_0);
  }
}


/**
  * @brief I2C MSP Initialization
  * @param hi2c I2C handle pointer
  * @retval None
  */
void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  if (hi2c->Instance == I2C1)
  {
    /*
     * I2C clock source
     */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
    PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;

    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /*
     * GPIO clocks
     */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /*
     * I2C1 GPIO Configuration
     *
     * PC14 ------> I2C1_SDA
     * PA9  ------> I2C1_SCL
     */

    /* SDA: PC14 */
    GPIO_InitStruct.Pin = GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF14_I2C1;

    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* SCL: PA9 */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF6_I2C1;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* I2C1 peripheral clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();
  }
}


/**
  * @brief I2C MSP De-Initialization
  * @param hi2c I2C handle pointer
  * @retval None
  */
void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c)
{
  if (hi2c->Instance == I2C1)
  {
    __HAL_RCC_I2C1_CLK_DISABLE();

    /*
     * I2C1 GPIO Configuration
     *
     * PC14 ------> I2C1_SDA
     * PA9  ------> I2C1_SCL
     */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_14);
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9);
  }
}


/**
  * @brief UART MSP Initialization
  * @param huart UART handle pointer
  * @retval None
  */
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if (huart->Instance == USART2)
  {
    /* USART2 peripheral clock enable */
    __HAL_RCC_USART2_CLK_ENABLE();

    /* GPIOA clock enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /*
     * USART2 GPIO Configuration
     *
     * PA2 ------> USART2_TX
     * PA3 ------> USART2_RX
     */
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_USART2;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
}


/**
  * @brief UART MSP De-Initialization
  * @param huart UART handle pointer
  * @retval None
  */
void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
  if (huart->Instance == USART2)
  {
    __HAL_RCC_USART2_CLK_DISABLE();

    /*
     * USART2 GPIO Configuration
     *
     * PA2 ------> USART2_TX
     * PA3 ------> USART2_RX
     */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2 | GPIO_PIN_3);
  }
}