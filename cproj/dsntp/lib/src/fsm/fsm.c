#include "dsntp/fsm.h"

void dsntp_fsm_init(dsntp_fsm_t *fsm) {
    if (fsm) fsm->state = DSNTP_ST_INIT;
}

static int can_trans(dsntp_state_t from, dsntp_event_t ev, dsntp_state_t *to) {
    switch (from) {
    case DSNTP_ST_INIT:
        if (ev == DSNTP_EV_START) { *to = DSNTP_ST_SYNC_WAIT; return 1; }
        break;
    case DSNTP_ST_SYNC_WAIT:
        if (ev == DSNTP_EV_TIMER_TICK) { *to = DSNTP_ST_COLLECTING; return 1; }
        if (ev == DSNTP_EV_EXT_TIME_VALID) { *to = DSNTP_ST_EXT_SYNC; return 1; }
        break;
    case DSNTP_ST_COLLECTING:
        if (ev == DSNTP_EV_COLLECT_DONE) { *to = DSNTP_ST_CONSENSUS; return 1; }
        break;
    case DSNTP_ST_CONSENSUS:
        if (ev == DSNTP_EV_CONSENSUS_DONE) { *to = DSNTP_ST_ESTIMATING; return 1; }
        break;
    case DSNTP_ST_ESTIMATING:
        if (ev == DSNTP_EV_ESTIMATE_DONE) { *to = DSNTP_ST_RUNNING; return 1; }
        break;
    case DSNTP_ST_RUNNING:
        if (ev == DSNTP_EV_TIMER_TICK) { *to = DSNTP_ST_SYNC_WAIT; return 1; }
        if (ev == DSNTP_EV_STOP) { *to = DSNTP_ST_STOPPED; return 1; }
        break;
    case DSNTP_ST_FAULT:
        if (ev == DSNTP_EV_RECOVER_DONE) { *to = DSNTP_ST_RECOVERING; return 1; }
        if (ev == DSNTP_EV_RESET) { *to = DSNTP_ST_SYNC_WAIT; return 1; }
        break;
    case DSNTP_ST_RECOVERING:
        if (ev == DSNTP_EV_ALIGN_SYNC_DONE) { *to = DSNTP_ST_SYNC_WAIT; return 1; }
        break;
    case DSNTP_ST_STOPPED:
        if (ev == DSNTP_EV_RESET) { *to = DSNTP_ST_SYNC_WAIT; return 1; }
        break;
    case DSNTP_ST_EXT_SYNC:
        if (ev == DSNTP_EV_EXT_SYNC_DONE) { *to = DSNTP_ST_SYNC_WAIT; return 1; }
        break;
    }
    if (ev == DSNTP_EV_LOCAL_FAULT && from != DSNTP_ST_FAULT && from != DSNTP_ST_STOPPED) {
        *to = DSNTP_ST_FAULT;
        return 1;
    }
    /* PeerFault*: legal event but does not change local state (FR-FSM-002) */
    if (ev == DSNTP_EV_PEER_FAULT_SUSPECT || ev == DSNTP_EV_PEER_FAULT_CONFIRM) {
        *to = from;
        return 1;
    }
    return 0;
}

dsntp_err_t dsntp_fsm_handle(dsntp_fsm_t *fsm, dsntp_event_t ev) {
    if (!fsm) return DSNTP_ERR_INVAL;
    dsntp_state_t to;
    if (!can_trans(fsm->state, ev, &to)) return DSNTP_ERR_STATE;
    fsm->state = to;
    return DSNTP_OK;
}

dsntp_state_t dsntp_fsm_state(const dsntp_fsm_t *fsm) {
    return fsm ? fsm->state : DSNTP_ST_INIT;
}

const char *dsntp_fsm_state_name(dsntp_state_t st) {
    static const char *names[] = {
        "Init", "SyncWait", "Collecting", "Consensus", "Estimating",
        "Running", "Fault", "Recovering", "Stopped", "Ext_Sync"
    };
    if ((unsigned)st < sizeof(names) / sizeof(names[0])) return names[st];
    return "?";
}
