/* payload.c — Type 0x01..0x08 big-endian payload codec (DRS §7.3) */
#include "dsntp/protocol.h"
#include <string.h>

/* declared in codec.c */
uint16_t dsntp_be16(uint16_t v);
uint32_t dsntp_be32(uint32_t v);
uint64_t dsntp_be64(uint64_t v);

static int need(size_t out_cap, size_t n) {
    return out_cap >= n ? DSNTP_OK : DSNTP_ERR_INVAL;
}

int dsntp_pl_time_req_encode(const dsntp_pl_time_req_t *p, uint8_t *out, size_t out_cap, size_t *out_len) {
    if (!p || !out || !out_len || need(out_cap, 12) != DSNTP_OK) return DSNTP_ERR_INVAL;
    uint32_t rid = dsntp_be32(p->request_id);
    uint64_t t0 = dsntp_be64(p->t0);
    memcpy(out, &rid, 4);
    memcpy(out + 4, &t0, 8);
    *out_len = 12;
    return DSNTP_OK;
}

int dsntp_pl_time_req_decode(const uint8_t *in, size_t len, dsntp_pl_time_req_t *p) {
    if (!in || !p || len < 12) return DSNTP_ERR_INVAL;
    uint32_t rid;
    uint64_t t0;
    memcpy(&rid, in, 4);
    memcpy(&t0, in + 4, 8);
    p->request_id = dsntp_be32(rid);
    p->t0 = dsntp_be64(t0);
    return DSNTP_OK;
}

int dsntp_pl_time_resp_encode(const dsntp_pl_time_resp_t *p, uint8_t *out, size_t out_cap, size_t *out_len) {
    if (!p || !out || !out_len || need(out_cap, 32) != DSNTP_OK) return DSNTP_ERR_INVAL;
    uint32_t rid = dsntp_be32(p->request_id);
    uint16_t rsp = dsntp_be16(p->responder_id);
    uint16_t z = 0;
    uint64_t t0 = dsntp_be64(p->t0), t1 = dsntp_be64(p->t1), t2 = dsntp_be64(p->t2);
    memcpy(out, &rid, 4);
    memcpy(out + 4, &rsp, 2);
    memcpy(out + 6, &z, 2);
    memcpy(out + 8, &t0, 8);
    memcpy(out + 16, &t1, 8);
    memcpy(out + 24, &t2, 8);
    *out_len = 32;
    return DSNTP_OK;
}

int dsntp_pl_time_resp_decode(const uint8_t *in, size_t len, dsntp_pl_time_resp_t *p) {
    if (!in || !p || len < 32) return DSNTP_ERR_INVAL;
    uint32_t rid;
    uint16_t rsp;
    uint64_t t0, t1, t2;
    memcpy(&rid, in, 4);
    memcpy(&rsp, in + 4, 2);
    memcpy(&t0, in + 8, 8);
    memcpy(&t1, in + 16, 8);
    memcpy(&t2, in + 24, 8);
    p->request_id = dsntp_be32(rid);
    p->responder_id = dsntp_be16(rsp);
    p->reserved = 0;
    p->t0 = dsntp_be64(t0);
    p->t1 = dsntp_be64(t1);
    p->t2 = dsntp_be64(t2);
    return DSNTP_OK;
}

int dsntp_pl_consensus_result_encode(const dsntp_pl_consensus_result_t *p, uint8_t *out, size_t out_cap, size_t *out_len) {
    if (!p || !out || !out_len || need(out_cap, 12) != DSNTP_OK) return DSNTP_ERR_INVAL;
    uint64_t tc = dsntp_be64(p->tc);
    uint16_t sc = dsntp_be16(p->sample_count);
    uint16_t z = 0;
    memcpy(out, &tc, 8);
    memcpy(out + 8, &sc, 2);
    memcpy(out + 10, &z, 2);
    *out_len = 12;
    return DSNTP_OK;
}

int dsntp_pl_consensus_result_decode(const uint8_t *in, size_t len, dsntp_pl_consensus_result_t *p) {
    if (!in || !p || len < 12) return DSNTP_ERR_INVAL;
    uint64_t tc;
    uint16_t sc;
    memcpy(&tc, in, 8);
    memcpy(&sc, in + 8, 2);
    p->tc = dsntp_be64(tc);
    p->sample_count = dsntp_be16(sc);
    p->reserved = 0;
    return DSNTP_OK;
}

int dsntp_pl_consensus_ack_encode(const dsntp_pl_consensus_ack_t *p, uint8_t *out, size_t out_cap, size_t *out_len) {
    if (!p || !out || !out_len || need(out_cap, 12) != DSNTP_OK) return DSNTP_ERR_INVAL;
    uint64_t tc = dsntp_be64(p->tc);
    memcpy(out, &tc, 8);
    out[8] = p->agree;
    out[9] = out[10] = out[11] = 0;
    *out_len = 12;
    return DSNTP_OK;
}

int dsntp_pl_consensus_ack_decode(const uint8_t *in, size_t len, dsntp_pl_consensus_ack_t *p) {
    if (!in || !p || len < 12) return DSNTP_ERR_INVAL;
    uint64_t tc;
    memcpy(&tc, in, 8);
    p->tc = dsntp_be64(tc);
    p->agree = in[8];
    memset(p->reserved, 0, sizeof(p->reserved));
    return DSNTP_OK;
}

int dsntp_pl_fault_notify_encode(const dsntp_pl_fault_notify_t *p, uint8_t *out, size_t out_cap, size_t *out_len) {
    if (!p || !out || !out_len || need(out_cap, 8) != DSNTP_OK) return DSNTP_ERR_INVAL;
    uint16_t id = dsntp_be16(p->fault_node_id);
    uint32_t rnd = dsntp_be32(p->detect_round);
    memcpy(out, &id, 2);
    out[2] = p->reason;
    out[3] = 0;
    memcpy(out + 4, &rnd, 4);
    *out_len = 8;
    return DSNTP_OK;
}

int dsntp_pl_fault_notify_decode(const uint8_t *in, size_t len, dsntp_pl_fault_notify_t *p) {
    if (!in || !p || len < 8) return DSNTP_ERR_INVAL;
    uint16_t id;
    uint32_t rnd;
    memcpy(&id, in, 2);
    memcpy(&rnd, in + 4, 4);
    p->fault_node_id = dsntp_be16(id);
    p->reason = in[2];
    p->reserved = 0;
    p->detect_round = dsntp_be32(rnd);
    return DSNTP_OK;
}

int dsntp_pl_start_sync_encode(const dsntp_pl_start_sync_t *p, uint8_t *out, size_t out_cap, size_t *out_len) {
    if (!p || !out || !out_len || need(out_cap, 14) != DSNTP_OK) return DSNTP_ERR_INVAL;
    uint16_t lid = dsntp_be16(p->leader_id);
    uint32_t period = dsntp_be32(p->period_ms);
    uint32_t dmax = dsntp_be32(p->dmax_ms);
    uint32_t sr = dsntp_be32(p->start_round);
    memcpy(out, &lid, 2);
    memcpy(out + 2, &period, 4);
    memcpy(out + 6, &dmax, 4);
    memcpy(out + 10, &sr, 4);
    *out_len = 14;
    return DSNTP_OK;
}

int dsntp_pl_start_sync_decode(const uint8_t *in, size_t len, dsntp_pl_start_sync_t *p) {
    if (!in || !p || len < 14) return DSNTP_ERR_INVAL;
    uint16_t lid;
    uint32_t period, dmax, sr;
    memcpy(&lid, in, 2);
    memcpy(&period, in + 2, 4);
    memcpy(&dmax, in + 6, 4);
    memcpy(&sr, in + 10, 4);
    p->leader_id = dsntp_be16(lid);
    p->period_ms = dsntp_be32(period);
    p->dmax_ms = dsntp_be32(dmax);
    p->start_round = dsntp_be32(sr);
    return DSNTP_OK;
}

int dsntp_pl_recover_announce_encode(const dsntp_pl_recover_announce_t *p, uint8_t *out, size_t out_cap, size_t *out_len) {
    if (!p || !out || !out_len || need(out_cap, 28) != DSNTP_OK) return DSNTP_ERR_INVAL;
    uint16_t id = dsntp_be16(p->node_id);
    uint16_t z = 0;
    uint64_t beta = dsntp_be64((uint64_t)p->beta_ns);
    uint64_t alpha = dsntp_be64((uint64_t)p->alpha_ppm);
    uint64_t tc = dsntp_be64(p->target_tc);
    memcpy(out, &id, 2);
    memcpy(out + 2, &z, 2);
    memcpy(out + 4, &beta, 8);
    memcpy(out + 12, &alpha, 8);
    memcpy(out + 20, &tc, 8);
    *out_len = 28;
    return DSNTP_OK;
}

int dsntp_pl_recover_announce_decode(const uint8_t *in, size_t len, dsntp_pl_recover_announce_t *p) {
    if (!in || !p || len < 28) return DSNTP_ERR_INVAL;
    uint16_t id;
    uint64_t beta, alpha, tc;
    memcpy(&id, in, 2);
    memcpy(&beta, in + 4, 8);
    memcpy(&alpha, in + 12, 8);
    memcpy(&tc, in + 20, 8);
    p->node_id = dsntp_be16(id);
    p->reserved = 0;
    p->beta_ns = (int64_t)dsntp_be64(beta);
    p->alpha_ppm = (int64_t)dsntp_be64(alpha);
    p->target_tc = dsntp_be64(tc);
    return DSNTP_OK;
}

int dsntp_pl_recover_ack_encode(const dsntp_pl_recover_ack_t *p, uint8_t *out, size_t out_cap, size_t *out_len) {
    if (!p || !out || !out_len || need(out_cap, 4) != DSNTP_OK) return DSNTP_ERR_INVAL;
    uint16_t id = dsntp_be16(p->node_id);
    memcpy(out, &id, 2);
    out[2] = p->accept;
    out[3] = 0;
    *out_len = 4;
    return DSNTP_OK;
}

int dsntp_pl_recover_ack_decode(const uint8_t *in, size_t len, dsntp_pl_recover_ack_t *p) {
    if (!in || !p || len < 4) return DSNTP_ERR_INVAL;
    uint16_t id;
    memcpy(&id, in, 2);
    p->node_id = dsntp_be16(id);
    p->accept = in[2];
    p->reserved = 0;
    return DSNTP_OK;
}
