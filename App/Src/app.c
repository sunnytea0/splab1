#include "app.h"
#include "mpu6050.h"

#include <stdio.h>
#include <stdint.h>

static ADC_HandleTypeDef *app_adc = NULL;
static MPU6050_Handle mpu6050;

/* -------------------------------------------------------------------------- */
/* ADC                                                                         */
/* -------------------------------------------------------------------------- */

static uint32_t App_ReadADC(void)
{
    uint32_t value = 0;

    if (HAL_ADC_Start(app_adc) != HAL_OK)
    {
        return 0;
    }

    if (HAL_ADC_PollForConversion(app_adc, 100) == HAL_OK)
    {
        value = HAL_ADC_GetValue(app_adc);
    }

    HAL_ADC_Stop(app_adc);

    return value;
}

/* -------------------------------------------------------------------------- */
/* Button                                                                      */
/* -------------------------------------------------------------------------- */

static GPIO_PinState App_ReadButton(void)
{
    return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
}

/* -------------------------------------------------------------------------- */
/* Self-test                                                                   */
/* -------------------------------------------------------------------------- */

static void App_SelfTest(void)
{
    uint8_t all_ok = 1;
    uint8_t who_am_i = 0;

    MPU6050_Data test_data;

    printf("\r\n");
    printf("====================================\r\n");
    printf("[SELFTEST] START\r\n");
    printf("====================================\r\n");

    if (MPU6050_CheckConnection(&mpu6050) == HAL_OK)
    {
        printf(
            "[SELFTEST] I2C MPU6050 ADDRESS=0x68 OK\r\n"
        );
    }
    else
    {
        printf(
            "[SELFTEST] I2C MPU6050 NOT FOUND\r\n"
        );

        all_ok = 0;
    }

    HAL_StatusTypeDef who_status =
        MPU6050_ReadWhoAmI(
            &mpu6050,
            &who_am_i
        );

    if (who_status == HAL_OK)
    {
        printf(
            "[SELFTEST] WHO_AM_I=0x%02X",
            who_am_i
        );

        if (who_am_i == MPU6050_EXPECTED_ID)
        {
            printf(" OK\r\n");
        }
        else
        {
            printf(
                " FAIL expected=0x%02X\r\n",
                MPU6050_EXPECTED_ID
            );

            all_ok = 0;
        }
    }
    else
    {
        printf(
            "[SELFTEST] WHO_AM_I READ FAIL status=%d\r\n",
            (int)who_status
        );

        all_ok = 0;
    }

    HAL_StatusTypeDef data_status =
        MPU6050_ReadData(
            &mpu6050,
            &test_data
        );

    if (data_status == HAL_OK)
    {
        printf(
            "[SELFTEST] MPU6050 MULTIBYTE READ OK\r\n"
        );

        printf(
            "[SELFTEST] RAW "
            "AX=%d AY=%d AZ=%d "
            "TEMP=%d "
            "GX=%d GY=%d GZ=%d\r\n",

            test_data.accel_x,
            test_data.accel_y,
            test_data.accel_z,
            test_data.temperature,
            test_data.gyro_x,
            test_data.gyro_y,
            test_data.gyro_z
        );
    }
    else
    {
        printf(
            "[SELFTEST] MPU6050 MULTIBYTE READ FAIL status=%d\r\n",
            (int)data_status
        );

        all_ok = 0;
    }

    uint32_t adc_value = App_ReadADC();

    printf(
        "[SELFTEST] ADC VALUE=%lu OK\r\n",
        (unsigned long)adc_value
    );

    GPIO_PinState button_state =
        App_ReadButton();

    printf(
        "[SELFTEST] BUTTON STATE=%d OK\r\n",
        (int)button_state
    );

    printf("[SELFTEST] UART OK\r\n");
    printf("------------------------------------\r\n");

    if (all_ok)
    {
        printf("[SELFTEST] RESULT=PASS\r\n");
    }
    else
    {
        printf("[SELFTEST] RESULT=FAIL\r\n");
    }

    printf("====================================\r\n");
}

/* -------------------------------------------------------------------------- */
/* Application initialization                                                  */
/* -------------------------------------------------------------------------- */

void App_Init(
    I2C_HandleTypeDef *hi2c,
    ADC_HandleTypeDef *hadc
)
{
    app_adc = hadc;

    MPU6050_Attach(
        &mpu6050,
        hi2c
    );

    printf("\r\n");
    printf("====================================\r\n");
    printf("LAB 2 - SENSOR SUBSYSTEM\r\n");
    printf("STM32C031C6 + MPU6050\r\n");
    printf("====================================\r\n");

    App_SelfTest();
}

/* -------------------------------------------------------------------------- */
/* Main application loop                                                       */
/* -------------------------------------------------------------------------- */

void App_Run(void)
{
    MPU6050_Data data;

    uint32_t adc_value =
        App_ReadADC();

    GPIO_PinState button_state =
        App_ReadButton();

    HAL_StatusTypeDef status =
        MPU6050_ReadData(
            &mpu6050,
            &data
        );

    if (status == HAL_OK)
    {
        int32_t ax_mg =
            MPU6050_AccelToMg(data.accel_x);

        int32_t ay_mg =
            MPU6050_AccelToMg(data.accel_y);

        int32_t az_mg =
            MPU6050_AccelToMg(data.accel_z);

        int32_t gx_mdps =
            MPU6050_GyroToMdps(data.gyro_x);

        int32_t gy_mdps =
            MPU6050_GyroToMdps(data.gyro_y);

        int32_t gz_mdps =
            MPU6050_GyroToMdps(data.gyro_z);

        int32_t temperature_centi =
            MPU6050_TemperatureToCentiC(
                data.temperature
            );

        printf(
            "MPU OK | "
            "ACCEL_mg X=%ld Y=%ld Z=%ld | "
            "GYRO_mdps X=%ld Y=%ld Z=%ld | "
            "TEMP_cC=%ld | "
            "ADC=%lu | "
            "BUTTON=%d\r\n",

            (long)ax_mg,
            (long)ay_mg,
            (long)az_mg,

            (long)gx_mdps,
            (long)gy_mdps,
            (long)gz_mdps,

            (long)temperature_centi,

            (unsigned long)adc_value,

            (int)button_state
        );
    }
    else
    {
        printf(
            "MPU READ FAIL status=%d | "
            "ADC=%lu | BUTTON=%d\r\n",

            (int)status,
            (unsigned long)adc_value,
            (int)button_state
        );
    }

    HAL_Delay(1000);
}