#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

/*
 * Host-тести перетворення сирих значень MPU6050.
 * Ці функції повторюють формули драйвера і не потребують STM32 HAL.
 */

static int32_t accel_to_mg(int16_t raw)
{
    return ((int32_t)raw * 1000L) / 16384L;
}

static int32_t gyro_to_mdps(int16_t raw)
{
    return ((int32_t)raw * 1000L) / 131L;
}

static int32_t temperature_to_centi_c(int16_t raw)
{
    return (((int32_t)raw * 100L) / 340L) + 3653L;
}

static void check(const char *name, int32_t actual, int32_t expected)
{
    if (actual != expected)
    {
        printf("[FAIL] %s: expected=%ld actual=%ld\n",
               name, (long)expected, (long)actual);
        exit(1);
    }

    printf("[PASS] %s\n", name);
}

int main(void)
{
    printf("MPU6050 host tests\n");

    check("Accel 0 raw -> 0 mg",
          accel_to_mg(0), 0);

    check("Accel +1g -> +1000 mg",
          accel_to_mg(16384), 1000);

    check("Accel -1g -> -1000 mg",
          accel_to_mg(-16384), -1000);

    check("Gyro 0 raw -> 0 mdps",
          gyro_to_mdps(0), 0);

    check("Gyro +131 raw -> +1000 mdps",
          gyro_to_mdps(131), 1000);

    check("Gyro -131 raw -> -1000 mdps",
          gyro_to_mdps(-131), -1000);

    check("Wokwi temperature raw -4260 -> 2401 cC",
          temperature_to_centi_c(-4260), 2401);

    printf("[PASS] ALL TESTS\n");

    return 0;
}