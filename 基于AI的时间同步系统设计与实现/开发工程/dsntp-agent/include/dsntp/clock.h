/*
 * clock.h — 本地单调钟与合成时钟（FR-CLK / FR-SYN）
 */
#ifndef DSNTP_CLOCK_H
#define DSNTP_CLOCK_H

#include "dsntp/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dsntp_clock {
    dsntp_ns_t last_sync_c;
    double     alpha_hat;
    int64_t    beta_hat_ns;
} dsntp_clock_t;

void       dsntp_clock_init(dsntp_clock_t *clk);
dsntp_ns_t dsntp_clock_monotonic_ns(void);
void       dsntp_clock_update_estimates(dsntp_clock_t *clk, double alpha, int64_t beta_ns, dsntp_ns_t sync_c);
dsntp_ns_t dsntp_clock_synced_ns(const dsntp_clock_t *clk);

#ifdef __cplusplus
}
#endif

#endif /* DSNTP_CLOCK_H */
