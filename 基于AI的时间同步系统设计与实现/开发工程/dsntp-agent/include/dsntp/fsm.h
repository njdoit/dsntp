/*
 * fsm.h — 有限状态机（FR-FSM / DRS §6）；PeerFault ≠ LocalFault
 */
#ifndef DSNTP_FSM_H
#define DSNTP_FSM_H

#include "dsntp/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum dsntp_state {
    DSNTP_ST_INIT = 0,
    DSNTP_ST_SYNC_WAIT,
    DSNTP_ST_COLLECTING,
    DSNTP_ST_CONSENSUS,
    DSNTP_ST_ESTIMATING,
    DSNTP_ST_RUNNING,
    DSNTP_ST_FAULT,
    DSNTP_ST_RECOVERING,
    DSNTP_ST_STOPPED,
    DSNTP_ST_EXT_SYNC
} dsntp_state_t;

typedef enum dsntp_event {
    DSNTP_EV_START = 0,
    DSNTP_EV_TIMER_TICK,
    DSNTP_EV_COLLECT_DONE,
    DSNTP_EV_CONSENSUS_DONE,
    DSNTP_EV_ESTIMATE_DONE,
    DSNTP_EV_LOCAL_FAULT,
    DSNTP_EV_PEER_FAULT_SUSPECT,  /* 不改变本机状态 */
    DSNTP_EV_PEER_FAULT_CONFIRM,  /* 不改变本机状态；触发 FAULT_NOTIFY */
    DSNTP_EV_RECOVER_DONE,
    DSNTP_EV_ALIGN_SYNC_DONE,
    DSNTP_EV_STOP,
    DSNTP_EV_RESET,
    DSNTP_EV_EXT_TIME_VALID,
    DSNTP_EV_EXT_SYNC_DONE
} dsntp_event_t;

typedef struct dsntp_fsm {
    dsntp_state_t state;
} dsntp_fsm_t;

void          dsntp_fsm_init(dsntp_fsm_t *fsm);
dsntp_err_t   dsntp_fsm_handle(dsntp_fsm_t *fsm, dsntp_event_t ev);
dsntp_state_t dsntp_fsm_state(const dsntp_fsm_t *fsm);
const char   *dsntp_fsm_state_name(dsntp_state_t st);

#ifdef __cplusplus
}
#endif

#endif /* DSNTP_FSM_H */
