/* perf.h — вимірювальний модуль для STM32C031 (Cortex-M0+).
 * У Cortex-M0+ немає DWT/CYCCNT, тому замість нього — мкс-лічильник
 * на SysTick + HAL tick (роздільність ~1 мкс). */
#ifndef PERF_H
#define PERF_H

#include "stm32c0xx_hal.h"
#include <stdint.h>

/* службові піни для логічного аналізатора (GPIOA) */
#define PROBE_IMU_PIN    6U   /* PA6: висока під час задачі IMU        */
#define PROBE_LONG_PIN   7U   /* PA7: висока під час довгої операції   */
#define PROBE_BTN_PIN    8U   /* PA8: висока під час реакції на кнопку */

#define PROBE_ON(n)   (GPIOA->BSRR = (1UL << (n)))
#define PROBE_OFF(n)  (GPIOA->BSRR = (1UL << ((n) + 16U)))

void     perf_init(void);        /* після SystemClock_Config() */
uint32_t perf_us(void);          /* мкс від старту, переповнення ~71 хв */

static inline uint32_t perf_elapsed_us(uint32_t t0) { return perf_us() - t0; }

void     stack_paint(void);      /* ПЕРШИМ рядком main() */
uint32_t stack_used_bytes(void);
uint32_t stack_reserved_bytes(void);
uint32_t heap_used_bytes(void);

#endif /* PERF_H */