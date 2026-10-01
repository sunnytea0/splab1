#include "mpu6050.h"

static HAL_StatusTypeDef MPU6050_ReadRegister(
    MPU6050_Handle *device,
    uint8_t reg,
    uint8_t *value
)
{
    HAL_StatusTypeDef status;

    status = HAL_I2C_Master_Transmit(
        device->hi2c,
        MPU6050_I2C_ADDRESS,
        &reg,
        1,
        100
    );

    if (status != HAL_OK)
    {
        return status;
    }

    return HAL_I2C_Master_Receive(
        device->hi2c,
        MPU6050_I2C_ADDRESS,
        value,
        1,
        100
    );
}

static HAL_StatusTypeDef MPU6050_ReadRegisters(
    MPU6050_Handle *device,
    uint8_t start_reg,
    uint8_t *buffer,
    uint16_t length
)
{
    HAL_StatusTypeDef status;

    status = HAL_I2C_Master_Transmit(
        device->hi2c,
        MPU6050_I2C_ADDRESS,
        &start_reg,
        1,
        100
    );

    if (status != HAL_OK)
    {
        return status;
    }

    return HAL_I2C_Master_Receive(
        device->hi2c,
        MPU6050_I2C_ADDRESS,
        buffer,
        length,
        100
    );
}

void MPU6050_Attach(
    MPU6050_Handle *device,
    I2C_HandleTypeDef *hi2c
)
{
    device->hi2c = hi2c;
}

HAL_StatusTypeDef MPU6050_CheckConnection(
    MPU6050_Handle *device
)
{
    return HAL_I2C_IsDeviceReady(
        device->hi2c,
        MPU6050_I2C_ADDRESS,
        3,
        100
    );
}

HAL_StatusTypeDef MPU6050_ReadWhoAmI(
    MPU6050_Handle *device,
    uint8_t *id
)
{
    return MPU6050_ReadRegister(
        device,
        MPU6050_WHO_AM_I_REG,
        id
    );
}

HAL_StatusTypeDef MPU6050_ReadData(
    MPU6050_Handle *device,
    MPU6050_Data *data
)
{
    uint8_t buffer[14];

    HAL_StatusTypeDef status =
        MPU6050_ReadRegisters(
            device,
            MPU6050_ACCEL_XOUT_H_REG,
            buffer,
            sizeof(buffer)
        );

    if (status != HAL_OK)
    {
        return status;
    }

    data->accel_x =
        (int16_t)(((uint16_t)buffer[0] << 8) | buffer[1]);

    data->accel_y =
        (int16_t)(((uint16_t)buffer[2] << 8) | buffer[3]);

    data->accel_z =
        (int16_t)(((uint16_t)buffer[4] << 8) | buffer[5]);

    data->temperature =
        (int16_t)(((uint16_t)buffer[6] << 8) | buffer[7]);

    data->gyro_x =
        (int16_t)(((uint16_t)buffer[8] << 8) | buffer[9]);

    data->gyro_y =
        (int16_t)(((uint16_t)buffer[10] << 8) | buffer[11]);

    data->gyro_z =
        (int16_t)(((uint16_t)buffer[12] << 8) | buffer[13]);

    return HAL_OK;
}

int32_t MPU6050_AccelToMg(int16_t raw)
{
    return ((int32_t)raw * 1000L) / 16384L;
}

int32_t MPU6050_GyroToMdps(int16_t raw)
{
    return ((int32_t)raw * 1000L) / 131L;
}

int32_t MPU6050_TemperatureToCentiC(int16_t raw)
{
    return (((int32_t)raw * 100L) / 340L) + 3653L;
}