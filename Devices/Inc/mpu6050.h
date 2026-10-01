#ifndef MPU6050_H
#define MPU6050_H

#include "stm32c0xx_hal.h"
#include <stdint.h>

#define MPU6050_I2C_ADDRESS       (0x68U << 1)
#define MPU6050_WHO_AM_I_REG      0x75U
#define MPU6050_ACCEL_XOUT_H_REG  0x3BU
#define MPU6050_EXPECTED_ID       0x68U

typedef struct
{
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;

    int16_t temperature;

    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
} MPU6050_Data;

typedef struct
{
    I2C_HandleTypeDef *hi2c;
} MPU6050_Handle;

void MPU6050_Attach(
    MPU6050_Handle *device,
    I2C_HandleTypeDef *hi2c
);

HAL_StatusTypeDef MPU6050_CheckConnection(
    MPU6050_Handle *device
);

HAL_StatusTypeDef MPU6050_ReadWhoAmI(
    MPU6050_Handle *device,
    uint8_t *id
);

HAL_StatusTypeDef MPU6050_ReadData(
    MPU6050_Handle *device,
    MPU6050_Data *data
);

int32_t MPU6050_AccelToMg(int16_t raw);
int32_t MPU6050_GyroToMdps(int16_t raw);
int32_t MPU6050_TemperatureToCentiC(int16_t raw);

#endif