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

int main(void) {
    assert(DSNTP_TYPE_FAULT_NOTIFY == 0x06);
    assert(DSNTP_SIGNATURE_SIZE == 64);
    test_header_roundtrip();
    test_fsm_peer_fault_no_transit();
    test_median_quorum();
    test_measure();
    printf("test_smoke OK\n");
    return 0;
}
