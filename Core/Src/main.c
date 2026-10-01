#include "main.h"
#include "app.h"

#include <stdio.h>
#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* Peripheral handles                                                         */
/* -------------------------------------------------------------------------- */

ADC_HandleTypeDef hadc1;
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart2;

/* -------------------------------------------------------------------------- */
/* Prototypes                                                                 */
/* -------------------------------------------------------------------------- */

void SystemClock_Config(void);

static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);

/* -------------------------------------------------------------------------- */
/* printf -> USART2                                                           */
/* -------------------------------------------------------------------------- */

int _write(int fd, char *p, int n)
{
    (void)fd;

    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)p,
        (uint16_t)n,
        HAL_MAX_DELAY
    );

    return n;
}

/* -------------------------------------------------------------------------- */
/* Main                                                                       */
/* -------------------------------------------------------------------------- */

int main(void)
{
    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_I2C1_Init();
    MX_USART2_UART_Init();

    App_Init(
        &hi2c1,
        &hadc1
    );

    while (1)
    {
        App_Run();
    }
}

/* -------------------------------------------------------------------------- */
/* System Clock                                                               */
/* -------------------------------------------------------------------------- */

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_FLASH_SET_LATENCY(
        FLASH_LATENCY_0
    );

    RCC_OscInitStruct.OscillatorType =
        RCC_OSCILLATORTYPE_HSI;

    RCC_OscInitStruct.HSIState =
        RCC_HSI_ON;

    RCC_OscInitStruct.HSIDiv =
        RCC_HSI_DIV4;

    RCC_OscInitStruct.HSICalibrationValue =
        RCC_HSICALIBRATION_DEFAULT;

    if (HAL_RCC_OscConfig(
            &RCC_OscInitStruct
        ) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1;

    RCC_ClkInitStruct.SYSCLKSource =
        RCC_SYSCLKSOURCE_HSI;

    RCC_ClkInitStruct.SYSCLKDivider =
        RCC_SYSCLK_DIV1;

    RCC_ClkInitStruct.AHBCLKDivider =
        RCC_HCLK_DIV1;

    RCC_ClkInitStruct.APB1CLKDivider =
        RCC_APB1_DIV1;

    if (HAL_RCC_ClockConfig(
            &RCC_ClkInitStruct,
            FLASH_LATENCY_0
        ) != HAL_OK)
    {
        Error_Handler();
    }
}

/* -------------------------------------------------------------------------- */
/* ADC1 Initialization                                                        */
/* -------------------------------------------------------------------------- */

static void MX_ADC1_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    hadc1.Instance = ADC1;

    hadc1.Init.ClockPrescaler =
        ADC_CLOCK_SYNC_PCLK_DIV1;

    hadc1.Init.Resolution =
        ADC_RESOLUTION_12B;

    hadc1.Init.DataAlign =
        ADC_DATAALIGN_RIGHT;

    hadc1.Init.ScanConvMode =
        ADC_SCAN_SEQ_FIXED;

    hadc1.Init.EOCSelection =
        ADC_EOC_SINGLE_CONV;

    hadc1.Init.LowPowerAutoWait =
        DISABLE;

    hadc1.Init.LowPowerAutoPowerOff =
        DISABLE;

    hadc1.Init.ContinuousConvMode =
        DISABLE;

    hadc1.Init.NbrOfConversion =
        1;

    hadc1.Init.DiscontinuousConvMode =
        DISABLE;

    hadc1.Init.ExternalTrigConv =
        ADC_SOFTWARE_START;

    hadc1.Init.ExternalTrigConvEdge =
        ADC_EXTERNALTRIGCONVEDGE_NONE;

    hadc1.Init.DMAContinuousRequests =
        DISABLE;

    hadc1.Init.Overrun =
        ADC_OVR_DATA_PRESERVED;

    hadc1.Init.SamplingTimeCommon1 =
        ADC_SAMPLETIME_1CYCLE_5;

    hadc1.Init.OversamplingMode =
        DISABLE;

    hadc1.Init.TriggerFrequencyMode =
        ADC_TRIGGER_FREQ_HIGH;

    if (HAL_ADC_Init(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }

    sConfig.Channel =
        ADC_CHANNEL_0;

    sConfig.Rank =
        ADC_RANK_CHANNEL_NUMBER;

    if (HAL_ADC_ConfigChannel(
            &hadc1,
            &sConfig
        ) != HAL_OK)
    {
        Error_Handler();
    }
}

/* -------------------------------------------------------------------------- */
/* I2C1 Initialization                                                        */
/* -------------------------------------------------------------------------- */

static void MX_I2C1_Init(void)
{
    hi2c1.Instance =
        I2C1;

    hi2c1.Init.Timing =
        0x00402D41;

    hi2c1.Init.OwnAddress1 =
        0;

    hi2c1.Init.AddressingMode =
        I2C_ADDRESSINGMODE_7BIT;

    hi2c1.Init.DualAddressMode =
        I2C_DUALADDRESS_DISABLE;

    hi2c1.Init.OwnAddress2 =
        0;

    hi2c1.Init.OwnAddress2Masks =
        I2C_OA2_NOMASK;

    hi2c1.Init.GeneralCallMode =
        I2C_GENERALCALL_DISABLE;

    hi2c1.Init.NoStretchMode =
        I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_I2CEx_ConfigAnalogFilter(
            &hi2c1,
            I2C_ANALOGFILTER_ENABLE
        ) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_I2CEx_ConfigDigitalFilter(
            &hi2c1,
            0
        ) != HAL_OK)
    {
        Error_Handler();
    }
}

/* -------------------------------------------------------------------------- */
/* USART2 Initialization                                                      */
/* -------------------------------------------------------------------------- */

static void MX_USART2_UART_Init(void)
{
    huart2.Instance =
        USART2;

    huart2.Init.BaudRate =
        115200;

    huart2.Init.WordLength =
        UART_WORDLENGTH_8B;

    huart2.Init.StopBits =
        UART_STOPBITS_1;

    huart2.Init.Parity =
        UART_PARITY_NONE;

    huart2.Init.Mode =
        UART_MODE_TX_RX;

    huart2.Init.HwFlowCtl =
        UART_HWCONTROL_NONE;

    huart2.Init.OverSampling =
        UART_OVERSAMPLING_16;

    huart2.Init.OneBitSampling =
        UART_ONE_BIT_SAMPLE_DISABLE;

    huart2.Init.ClockPrescaler =
        UART_PRESCALER_DIV1;

    huart2.AdvancedInit.AdvFeatureInit =
        UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}

/* -------------------------------------------------------------------------- */
/* GPIO Initialization                                                        */
/* -------------------------------------------------------------------------- */

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin =
        GPIO_PIN_0;

    GPIO_InitStruct.Mode =
        GPIO_MODE_INPUT;

    GPIO_InitStruct.Pull =
        GPIO_NOPULL;

    HAL_GPIO_Init(
        GPIOB,
        &GPIO_InitStruct
    );
}

/* -------------------------------------------------------------------------- */
/* Error Handler                                                              */
/* -------------------------------------------------------------------------- */

void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
}

#ifdef USE_FULL_ASSERT

void assert_failed(
    uint8_t *file,
    uint32_t line
)
{
    (void)file;
    (void)line;
}

#endif