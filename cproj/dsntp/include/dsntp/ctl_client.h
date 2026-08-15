/*
 * ctl_client.h — Reporter -> Control Server (IF-CTL HTTP JSON, M4 stub)
 * Must NOT be called from UDP consensus hot path (FR-ARCH-004).
 */
#ifndef DSNTP_CTL_CLIENT_H
#define DSNTP_CTL_CLIENT_H

#include "dsntp/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dsntp_report_snapshot {
    dsntp_node_id_t node_id;
    uint64_t        seq;
    dsntp_ns_t      reported_at_ns;
    dsntp_ns_t      monotonic_ns;
    dsntp_ns_t      synced_ns;
    dsntp_ns_t      consensus_tc;
    dsntp_round_t   round;
    const char     *fsm_state;
    double          max_peer_offset_ms;
    double          rtt_avg_ms;
} dsntp_report_snapshot_t;

/* POST /api/v1/ingest/report — stub returns OK without I/O if endpoint empty */
dsntp_err_t dsntp_ctl_post_report(const char *server_endpoint,
                                  const dsntp_report_snapshot_t *snap);

#ifdef __cplusplus
}
#endif

#endif /* DSNTP_CTL_CLIENT_H */
