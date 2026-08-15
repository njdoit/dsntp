/*
 * reporter.c — control-plane reporter stub (FR-CTL / FR-ARCH-004)
 * Runs off the UDP hot path; snapshot only.
 */
#include "dsntp/ctl_client.h"
#include "dsntp/clock.h"
#include "dsntp/fsm.h"
#include "dsntp/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(_WIN32)
#include <pthread.h>
#include <unistd.h>
#endif

typedef struct dsntp_reporter {
    volatile int stop;
    const dsntp_config_t *cfg;
    const dsntp_clock_t  *clk;
    const dsntp_fsm_t    *fsm;
    const dsntp_ns_t     *tc;
    const dsntp_round_t  *round;
#if !defined(_WIN32)
    pthread_t tid;
    int       started;
#endif
} dsntp_reporter_t;

dsntp_err_t dsntp_ctl_post_report(const char *server_endpoint,
                                  const dsntp_report_snapshot_t *snap) {
    if (!snap) return DSNTP_ERR_INVAL;
    if (!server_endpoint || server_endpoint[0] == '\0') return DSNTP_OK;
    /* Scaffold: log-only; M4 wires real HTTP POST */
    fprintf(stderr,
            "[reporter] POST %s/api/v1/ingest/report node=%u round=%u synced=%llu fsm=%s\n",
            server_endpoint,
            (unsigned)snap->node_id,
            (unsigned)snap->round,
            (unsigned long long)snap->synced_ns,
            snap->fsm_state ? snap->fsm_state : "?");
    return DSNTP_OK;
}

static void reporter_once(dsntp_reporter_t *r) {
    if (!r || !r->cfg || !r->clk || !r->fsm) return;
    dsntp_report_snapshot_t snap;
    memset(&snap, 0, sizeof(snap));
    snap.node_id = r->cfg->node_id;
    snap.seq = 0;
    snap.reported_at_ns = dsntp_clock_monotonic_ns();
    snap.monotonic_ns = snap.reported_at_ns;
    snap.synced_ns = dsntp_clock_synced_ns(r->clk);
    snap.consensus_tc = r->tc ? *r->tc : 0;
    snap.round = r->round ? *r->round : 0;
    snap.fsm_state = dsntp_fsm_state_name(dsntp_fsm_state(r->fsm));
    dsntp_ctl_post_report(r->cfg->server_endpoint, &snap);
}

#if !defined(_WIN32)
static void *reporter_thread(void *arg) {
    dsntp_reporter_t *r = (dsntp_reporter_t *)arg;
    while (!r->stop) {
        reporter_once(r);
        sleep(1); /* Metrics period default 1s (FR-CTL-003) */
    }
    return NULL;
}
#endif

dsntp_reporter_t *dsntp_reporter_start(const dsntp_config_t *cfg,
                                       const dsntp_clock_t *clk,
                                       const dsntp_fsm_t *fsm,
                                       const dsntp_ns_t *tc,
                                       const dsntp_round_t *round) {
    dsntp_reporter_t *r = (dsntp_reporter_t *)calloc(1, sizeof(*r));
    if (!r) return NULL;
    r->cfg = cfg;
    r->clk = clk;
    r->fsm = fsm;
    r->tc = tc;
    r->round = round;
#if !defined(_WIN32)
    if (pthread_create(&r->tid, NULL, reporter_thread, r) == 0) r->started = 1;
#endif
    return r;
}

void dsntp_reporter_stop(dsntp_reporter_t *r) {
    if (!r) return;
    r->stop = 1;
#if !defined(_WIN32)
    if (r->started) pthread_join(r->tid, NULL);
#endif
    free(r);
}
