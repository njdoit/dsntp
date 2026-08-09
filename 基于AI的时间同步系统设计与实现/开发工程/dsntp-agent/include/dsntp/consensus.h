/*
 * consensus.h — 中位数共识（FR-CNS）
 */
#ifndef DSNTP_CONSENSUS_H
#define DSNTP_CONSENSUS_H

#include "dsntp/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dsntp_consensus {
    int        n;
    int        f;
    dsntp_ns_t tc;
    bool       tc_valid;
} dsntp_consensus_t;

void        dsntp_consensus_init(dsntp_consensus_t *c, int n, int f);
dsntp_err_t dsntp_consensus_median(dsntp_consensus_t *c, const dsntp_ns_t *samples, int count);
bool        dsntp_consensus_quorum_ok(const dsntp_consensus_t *c, int sample_count);
bool        dsntp_consensus_ack_quorum(const dsntp_consensus_t *c, int agree_count); /* ≥2f+1 */

#ifdef __cplusplus
}
#endif

#endif /* DSNTP_CONSENSUS_H */
