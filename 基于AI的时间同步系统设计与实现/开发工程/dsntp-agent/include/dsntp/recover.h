/*
 * recover.h — 故障剔除与恢复重入（FR-FLT）
 */
#ifndef DSNTP_RECOVER_H
#define DSNTP_RECOVER_H

#include "dsntp/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dsntp_peer_fault {
    dsntp_node_id_t node_id;
    int             miss_rounds;
    bool            confirmed;
    int             vote_count;
} dsntp_peer_fault_t;

typedef struct dsntp_recover {
    int  silent_rounds_left;
    bool announce_sent;
    int  ack_count;
} dsntp_recover_t;

void        dsntp_peer_fault_on_timeout(dsntp_peer_fault_t *pf, dsntp_node_id_t id);
bool        dsntp_peer_fault_confirm(dsntp_peer_fault_t *pf, int threshold_rounds);
void        dsntp_peer_fault_add_vote(dsntp_peer_fault_t *pf);
bool        dsntp_peer_fault_quorum(const dsntp_peer_fault_t *pf, int f);

void        dsntp_recover_begin(dsntp_recover_t *r, int silent_rounds);
bool        dsntp_recover_silent_done(const dsntp_recover_t *r);
void        dsntp_recover_tick_silent(dsntp_recover_t *r);
void        dsntp_recover_on_ack(dsntp_recover_t *r);
bool        dsntp_recover_ack_quorum(const dsntp_recover_t *r, int f); /* ≥2f+1 */

#ifdef __cplusplus
}
#endif

#endif /* DSNTP_RECOVER_H */
