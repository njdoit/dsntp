/*
 * measure.h — 双向时延与滑动窗口回归（FR-MEAS / FR-SYN-001）
 */
#ifndef DSNTP_MEASURE_H
#define DSNTP_MEASURE_H

#include "dsntp/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DSNTP_WINDOW_DEFAULT 10

typedef struct dsntp_sample {
    dsntp_ns_t t0, t1, t2, t3;
    int64_t    rtt_ns;
    int64_t    theta_ns;
    bool       valid;
} dsntp_sample_t;

typedef struct dsntp_window {
    int64_t theta[DSNTP_WINDOW_DEFAULT];
    int     count;
    int     cap;
} dsntp_window_t;

dsntp_err_t dsntp_measure_compute(dsntp_sample_t *s, dsntp_ns_t dmax_ns);
void        dsntp_window_init(dsntp_window_t *w, int cap);
void        dsntp_window_push(dsntp_window_t *w, int64_t theta_ns);
dsntp_err_t dsntp_window_regress(const dsntp_window_t *w, double *alpha_out, int64_t *beta_ns_out);

#ifdef __cplusplus
}
#endif

#endif /* DSNTP_MEASURE_H */
