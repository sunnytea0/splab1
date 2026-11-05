#ifndef PERF_STAT_H
#define PERF_STAT_H

#include <stdint.h>

typedef struct {
    uint32_t count;
    uint32_t min;
    uint32_t max;
    uint64_t sum;
} perf_stat_t;

#define PERF_HIST_BINS 8U

typedef struct {
    uint32_t edge[PERF_HIST_BINS - 1U]; /* верхні межі кошиків (exclusive) */
    uint32_t bin[PERF_HIST_BINS];       /* останній кошик: >= edge[last]   */
} perf_hist_t;

void     perf_stat_reset(perf_stat_t *s);
void     perf_stat_add(perf_stat_t *s, uint32_t v);
uint32_t perf_stat_avg(const perf_stat_t *s);

void perf_hist_init(perf_hist_t *h, const uint32_t edges[PERF_HIST_BINS - 1U]);
void perf_hist_reset(perf_hist_t *h);
void perf_hist_add(perf_hist_t *h, uint32_t v);

#endif /* PERF_STAT_H */