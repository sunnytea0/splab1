#ifndef APP_H
#define APP_H

#include "stm32c0xx_hal.h"

void App_Init(
    I2C_HandleTypeDef *hi2c,
    ADC_HandleTypeDef *hadc
);

void App_Run(void);

#endif