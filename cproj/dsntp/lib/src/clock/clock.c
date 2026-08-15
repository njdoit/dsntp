#include "dsntp/clock.h"
#include <time.h>

#if defined(_WIN32)
#include <windows.h>
#endif

void dsntp_clock_init(dsntp_clock_t *clk) {
    if (!clk) return;
    clk->last_sync_c = 0;
    clk->alpha_hat = 0.0;
    clk->beta_hat_ns = 0;
}

dsntp_ns_t dsntp_clock_monotonic_ns(void) {
#if defined(_WIN32)
    static LARGE_INTEGER freq;
    static int init = 0;
    LARGE_INTEGER c;
    if (!init) { QueryPerformanceFrequency(&freq); init = 1; }
    QueryPerformanceCounter(&c);
    return (dsntp_ns_t)((c.QuadPart * 1000000000ull) / (uint64_t)freq.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (dsntp_ns_t)ts.tv_sec * 1000000000ull + (dsntp_ns_t)ts.tv_nsec;
#endif
}

void dsntp_clock_update_estimates(dsntp_clock_t *clk, double alpha, int64_t beta_ns, dsntp_ns_t sync_c) {
    if (!clk) return;
    clk->alpha_hat = alpha;
    clk->beta_hat_ns = beta_ns;
    clk->last_sync_c = sync_c;
}

dsntp_ns_t dsntp_clock_synced_ns(const dsntp_clock_t *clk) {
    if (!clk) return 0;
    dsntp_ns_t now = dsntp_clock_monotonic_ns();
    dsntp_ns_t delta = (clk->last_sync_c == 0) ? 0 : (now - clk->last_sync_c);
    double corr = (double)clk->beta_hat_ns + clk->alpha_hat * (double)delta;
    return now + (dsntp_ns_t)corr;
}
