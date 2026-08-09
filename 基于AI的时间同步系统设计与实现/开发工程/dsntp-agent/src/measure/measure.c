#include "dsntp/measure.h"
#include <string.h>

dsntp_err_t dsntp_measure_compute(dsntp_sample_t *s, dsntp_ns_t dmax_ns) {
    if (!s) return DSNTP_ERR_INVAL;
    int64_t rtt = (int64_t)(s->t3 - s->t0) - (int64_t)(s->t2 - s->t1);
    int64_t theta = (int64_t)((s->t1 + s->t2) / 2) - (int64_t)((s->t0 + s->t3) / 2);
    s->rtt_ns = rtt;
    s->theta_ns = theta;
    s->valid = (rtt >= 0 && (dsntp_ns_t)rtt <= 2 * dmax_ns);
    return s->valid ? DSNTP_OK : DSNTP_ERR_TIMEOUT;
}

void dsntp_window_init(dsntp_window_t *w, int cap) {
    if (!w) return;
    memset(w, 0, sizeof(*w));
    w->cap = (cap > 0 && cap <= DSNTP_WINDOW_DEFAULT) ? cap : DSNTP_WINDOW_DEFAULT;
}

void dsntp_window_push(dsntp_window_t *w, int64_t theta_ns) {
    if (!w) return;
    if (w->count < w->cap) {
        w->theta[w->count++] = theta_ns;
    } else {
        memmove(&w->theta[0], &w->theta[1], (size_t)(w->cap - 1) * sizeof(int64_t));
        w->theta[w->cap - 1] = theta_ns;
    }
}

/* 简单最小二乘：theta ≈ beta + alpha * k（k=0..n-1） */
dsntp_err_t dsntp_window_regress(const dsntp_window_t *w, double *alpha_out, int64_t *beta_ns_out) {
    if (!w || !alpha_out || !beta_ns_out || w->count < 2) return DSNTP_ERR_INVAL;
    int n = w->count;
    double sum_x = 0, sum_y = 0, sum_xx = 0, sum_xy = 0;
    for (int i = 0; i < n; i++) {
        double x = (double)i;
        double y = (double)w->theta[i];
        sum_x += x; sum_y += y; sum_xx += x * x; sum_xy += x * y;
    }
    double den = (double)n * sum_xx - sum_x * sum_x;
    if (den == 0.0) return DSNTP_ERR_INVAL;
    double alpha = ((double)n * sum_xy - sum_x * sum_y) / den;
    double beta = (sum_y - alpha * sum_x) / (double)n;
    *alpha_out = alpha;
    *beta_ns_out = (int64_t)beta;
    return DSNTP_OK;
}
