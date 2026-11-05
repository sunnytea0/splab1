#ifndef APP_H
#define APP_H

#include "stm32c0xx_hal.h"
#include <stdint.h>

void App_Init(I2C_HandleTypeDef *hi2c,
              ADC_HandleTypeDef *hadc,
              UART_HandleTypeDef *huart);

void App_Run(void);

/* Викликається з ISR (EXTI0, спадаючий фронт кнопки). */
void App_OnButtonPressed(uint32_t now_ms);

#endif /* APP_H */