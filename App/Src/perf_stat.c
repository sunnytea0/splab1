#include "perf_stat.h"

#include <string.h>

void perf_stat_reset(perf_stat_t *s)
{
    s->count = 0U;
    s->min   = 0U;
    s->max   = 0U;
    s->sum   = 0U;
}

void perf_stat_add(perf_stat_t *s, uint32_t v)
{
    if ((s->count == 0U) || (v < s->min)) { s->min = v; }
    if ((s->count == 0U) || (v > s->max)) { s->max = v; }
    s->sum += v;
    s->count++;
}

uint32_t perf_stat_avg(const perf_stat_t *s)
{
    if (s->count == 0U) { return 0U; }
    return (uint32_t)(s->sum / s->count);
}

void perf_hist_init(perf_hist_t *h, const uint32_t edges[PERF_HIST_BINS - 1U])
{
    memcpy(h->edge, edges, sizeof(h->edge));
    perf_hist_reset(h);
}

void perf_hist_reset(perf_hist_t *h)
{
    memset(h->bin, 0, sizeof(h->bin));
}

void perf_hist_add(perf_hist_t *h, uint32_t v)
{
    for (uint32_t i = 0U; i < (PERF_HIST_BINS - 1U); i++)
    {
        if (v < h->edge[i])
        {
            h->bin[i]++;
            return;
        }
    }
    h->bin[PERF_HIST_BINS - 1U]++;
}