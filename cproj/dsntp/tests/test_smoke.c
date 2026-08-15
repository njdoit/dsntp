#include "dsntp/protocol.h"
#include "dsntp/fsm.h"
#include "dsntp/consensus.h"
#include "dsntp/measure.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static int test_header_roundtrip(void) {
    dsntp_header_t h = {0}, o;
    uint8_t buf[DSNTP_HEADER_SIZE];
    h.magic = DSNTP_MAGIC;
    h.version = DSNTP_VERSION;
    h.type = DSNTP_TYPE_FAULT_NOTIFY;
    h.flags = DSNTP_FLAG_URGENT;
    h.sender_id = 1;
    h.payload_len = 8;
    h.round = 52;
    assert(dsntp_header_encode(&h, buf) == DSNTP_OK);
    assert(dsntp_header_decode(buf, &o) == DSNTP_OK);
    assert(o.type == DSNTP_TYPE_FAULT_NOTIFY);
    assert(o.type != DSNTP_TYPE_TIME_RESP);
    assert(o.round == 52);
    return 0;
}

static int test_fsm_peer_fault_no_transit(void) {
    dsntp_fsm_t fsm;
    dsntp_fsm_init(&fsm);
    dsntp_fsm_handle(&fsm, DSNTP_EV_START);
    dsntp_fsm_handle(&fsm, DSNTP_EV_TIMER_TICK);
    assert(dsntp_fsm_state(&fsm) == DSNTP_ST_COLLECTING);
    assert(dsntp_fsm_handle(&fsm, DSNTP_EV_PEER_FAULT_CONFIRM) == DSNTP_OK);
    assert(dsntp_fsm_state(&fsm) == DSNTP_ST_COLLECTING);
    return 0;
}

static int test_median_quorum(void) {
    dsntp_consensus_t c;
    dsntp_ns_t s[3] = {300, 100, 200};
    dsntp_consensus_init(&c, 5, 1);
    assert(dsntp_consensus_median(&c, s, 2) == DSNTP_ERR_QUORUM);
    assert(dsntp_consensus_median(&c, s, 3) == DSNTP_OK);
    assert(c.tc == 200);
    return 0;
}

static int test_measure(void) {
    dsntp_sample_t s = {.t0 = 0, .t1 = 1000000, .t2 = 1000000, .t3 = 2000000};
    assert(dsntp_measure_compute(&s, 10000000) == DSNTP_OK);
    assert(s.valid);
    return 0;
}

static int test_alpha_ppm(void) {
    /* IF-TSYN-015: 0.047ppm -> AlphaPpm=47000; alpha = 0.047e-6 = 4.7e-8 */
    double a = dsntp_alpha_from_wire(47000);
    assert(a > 4.6e-8 && a < 4.8e-8);
    int64_t w = dsntp_alpha_to_wire(a);
    assert(w > 46000 && w < 48000);
    return 0;
}

static int test_time_req_payload(void) {
    dsntp_pl_time_req_t in = {.request_id = 7, .t0 = 0x0102030405060708ull};
    dsntp_pl_time_req_t out;
    uint8_t buf[32];
    size_t n = 0;
    assert(dsntp_pl_time_req_encode(&in, buf, sizeof(buf), &n) == DSNTP_OK);
    assert(n == 12);
    assert(dsntp_pl_time_req_decode(buf, n, &out) == DSNTP_OK);
    assert(out.request_id == 7);
    assert(out.t0 == 0x0102030405060708ull);
    return 0;
}

static int test_consensus_result_ack(void) {
    dsntp_pl_consensus_result_t rin = {.tc = 123456789ull, .sample_count = 3};
    dsntp_pl_consensus_result_t rout;
    dsntp_pl_consensus_ack_t ain = {.tc = 123456789ull, .agree = 1};
    dsntp_pl_consensus_ack_t aout;
    uint8_t buf[32];
    size_t n = 0;
    assert(dsntp_pl_consensus_result_encode(&rin, buf, sizeof(buf), &n) == DSNTP_OK);
    assert(dsntp_pl_consensus_result_decode(buf, n, &rout) == DSNTP_OK);
    assert(rout.tc == rin.tc && rout.sample_count == 3);
    assert(dsntp_pl_consensus_ack_encode(&ain, buf, sizeof(buf), &n) == DSNTP_OK);
    assert(dsntp_pl_consensus_ack_decode(buf, n, &aout) == DSNTP_OK);
    assert(aout.tc == ain.tc && aout.agree == 1);
    return 0;
}

static int test_window_regress(void) {
    dsntp_window_t w;
    dsntp_window_init(&w, 10);
    dsntp_window_push(&w, 1000);
    dsntp_window_push(&w, 1100);
    dsntp_window_push(&w, 1200);
    double alpha = 0;
    int64_t beta = 0;
    assert(dsntp_window_regress(&w, &alpha, &beta) == DSNTP_OK);
    assert(alpha > 90.0 && alpha < 110.0);
    return 0;
}

int main(void) {
    assert(DSNTP_TYPE_FAULT_NOTIFY == 0x06);
    assert(DSNTP_SIGNATURE_SIZE == 64);
    assert(DSNTP_MAGIC == 0x5453594Eu);
    test_header_roundtrip();
    test_fsm_peer_fault_no_transit();
    test_median_quorum();
    test_measure();
    test_alpha_ppm();
    test_time_req_payload();
    test_consensus_result_ack();
    test_window_regress();
    printf("test_smoke OK\n");
    return 0;
}
