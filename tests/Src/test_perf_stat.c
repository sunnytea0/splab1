#include "perf_stat.h"

#include <assert.h>
#include <stdio.h>

static void test_stat_basic(void)
{
    perf_stat_t s;
    perf_stat_reset(&s);
    assert(s.count == 0U && perf_stat_avg(&s) == 0U);

    perf_stat_add(&s, 100U);
    perf_stat_add(&s, 300U);
    perf_stat_add(&s, 200U);

    assert(s.count == 3U);
    assert(s.min == 100U);
    assert(s.max == 300U);
    assert(perf_stat_avg(&s) == 200U);
}

static void test_stat_first_sample_sets_min_max(void)
{
    perf_stat_t s;
    perf_stat_reset(&s);
    perf_stat_add(&s, 5000U);
    assert(s.min == 5000U && s.max == 5000U);
}

static void test_stat_no_overflow_32bit_sum(void)
{
    perf_stat_t s;
    perf_stat_reset(&s);
    for (int i = 0; i < 10; i++) { perf_stat_add(&s, 0xF0000000U); }
    assert(perf_stat_avg(&s) == 0xF0000000U); /* сума 64-бітна */
}

static void test_hist_bins(void)
{
    const uint32_t edges[PERF_HIST_BINS - 1U] = { 100U, 500U, 1000U, 5000U, 20000U, 50000U, 100000U };
    perf_hist_t h;
    perf_hist_init(&h, edges);

    perf_hist_add(&h, 0U);        /* bin0 */
    perf_hist_add(&h, 99U);       /* bin0 */
    perf_hist_add(&h, 100U);      /* bin1: межа exclusive */
    perf_hist_add(&h, 999U);      /* bin2 */
    perf_hist_add(&h, 99999U);    /* bin6 */
    perf_hist_add(&h, 100000U);   /* bin7 */
    perf_hist_add(&h, 5000000U);  /* bin7 */

    assert(h.bin[0] == 2U && h.bin[1] == 1U && h.bin[2] == 1U);
    assert(h.bin[6] == 1U && h.bin[7] == 2U);

    perf_hist_reset(&h);
    for (uint32_t i = 0U; i < PERF_HIST_BINS; i++) { assert(h.bin[i] == 0U); }
}

int main(void)
{
    test_stat_basic();
    test_stat_first_sample_sets_min_max();
    test_stat_no_overflow_32bit_sum();
    test_hist_bins();
    printf("test_perf_stat: all tests passed\n");
    return 0;
}