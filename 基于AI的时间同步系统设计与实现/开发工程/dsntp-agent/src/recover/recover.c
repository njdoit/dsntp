#include "dsntp/recover.h"

void dsntp_peer_fault_on_timeout(dsntp_peer_fault_t *pf, dsntp_node_id_t id) {
    if (!pf) return;
    if (pf->node_id != id) {
        pf->node_id = id;
        pf->miss_rounds = 0;
        pf->confirmed = false;
        pf->vote_count = 0;
    }
    pf->miss_rounds++;
}

bool dsntp_peer_fault_confirm(dsntp_peer_fault_t *pf, int threshold_rounds) {
    if (!pf) return false;
    if (pf->miss_rounds >= threshold_rounds) {
        pf->confirmed = true;
        return true;
    }
    return false;
}

void dsntp_peer_fault_add_vote(dsntp_peer_fault_t *pf) {
    if (pf) pf->vote_count++;
}

bool dsntp_peer_fault_quorum(const dsntp_peer_fault_t *pf, int f) {
    return pf && pf->vote_count >= (f + 1);
}

void dsntp_recover_begin(dsntp_recover_t *r, int silent_rounds) {
    if (!r) return;
    r->silent_rounds_left = silent_rounds > 0 ? silent_rounds : 2;
    r->announce_sent = false;
    r->ack_count = 0;
}

bool dsntp_recover_silent_done(const dsntp_recover_t *r) {
    return r && r->silent_rounds_left <= 0;
}

void dsntp_recover_tick_silent(dsntp_recover_t *r) {
    if (r && r->silent_rounds_left > 0) r->silent_rounds_left--;
}

void dsntp_recover_on_ack(dsntp_recover_t *r) {
    if (r) r->ack_count++;
}

bool dsntp_recover_ack_quorum(const dsntp_recover_t *r, int f) {
    return r && r->ack_count >= (2 * f + 1);
}
