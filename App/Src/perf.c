#include "perf.h"

static uint32_t perf_recip; /* 65536 / (такти на мкс): множення замість ділення */

void perf_init(void)
{
    uint32_t cyc_per_us = SystemCoreClock / 1000000U;
    if (cyc_per_us == 0U) { cyc_per_us = 1U; }
    perf_recip = 65536U / cyc_per_us;

    /* службові піни PA6, PA7, PA8 як виходи */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin   = (1U << PROBE_IMU_PIN) | (1U << PROBE_LONG_PIN) | (1U << PROBE_BTN_PIN);
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &g);
    GPIOA->BSRR = ((1UL << PROBE_IMU_PIN) | (1UL << PROBE_LONG_PIN) | (1UL << PROBE_BTN_PIN)) << 16U;
}

uint32_t perf_us(void)
{
    uint32_t ms, ms2, v1, v2, pend;

    /* VAL лише спадає, тому v2 > v1 означає перенос між читаннями — повторюємо. */
    do {
        ms   = HAL_GetTick();
        v1   = SysTick->VAL;
        pend = SCB->ICSR & SCB_ICSR_PENDSTSET_Msk; /* перенос був, ISR ще не виконано */
        v2   = SysTick->VAL;
        ms2  = HAL_GetTick();
    } while ((ms != ms2) || (v2 > v1));

    if (pend != 0U) { ms++; }

    uint32_t cyc = SysTick->LOAD - v1;
    return (ms * 1000U) + ((cyc * perf_recip) >> 16U);
}

/* --- пам'ять --- */

extern uint32_t _estack;
extern uint32_t _Min_Stack_Size;
extern uint8_t  _end;
void *_sbrk(int incr);

#define STACK_PATTERN 0xDEADBEEFUL

static uint32_t *stack_bottom(void)
{
    return (uint32_t *)((uint32_t)&_estack - (uint32_t)&_Min_Stack_Size);
}

void stack_paint(void)
{
    uint32_t *p  = stack_bottom();
    uint32_t *sp = (uint32_t *)__get_MSP();
    while (p < (sp - 16)) { *p++ = STACK_PATTERN; }
}

uint32_t stack_used_bytes(void)
{
    uint32_t *p = stack_bottom();
    while ((p < &_estack) && (*p == STACK_PATTERN)) { p++; }
    return (uint32_t)&_estack - (uint32_t)p;
}

uint32_t stack_reserved_bytes(void)
{
    return (uint32_t)&_Min_Stack_Size;
}

uint32_t heap_used_bytes(void)
{
    return (uint32_t)((uint8_t *)_sbrk(0) - &_end);
}