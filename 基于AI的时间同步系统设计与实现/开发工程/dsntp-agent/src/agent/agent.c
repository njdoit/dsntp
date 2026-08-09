#include "dsntp/agent.h"
#include "dsntp/clock.h"
#include "dsntp/fsm.h"
#include "dsntp/consensus.h"
#include "dsntp/measure.h"
#include "dsntp/recover.h"
#include "dsntp/net.h"
#include "dsntp/crypto.h"
#include "dsntp/api_local.h"
#include "dsntp/protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(_WIN32)
#include <unistd.h>
#else
#include <windows.h>
#endif

struct dsntp_agent {
    dsntp_config_t   cfg;
    dsntp_fsm_t      fsm;
    dsntp_clock_t    clock;
    dsntp_consensus_t consensus;
    dsntp_window_t   window;
    dsntp_net_t     *net;
    dsntp_crypto_t  *crypto;
    dsntp_api_local_t *api;
    volatile int     stop;
    dsntp_round_t    round;
};

dsntp_agent_t *dsntp_agent_create(const dsntp_config_t *cfg) {
    if (!cfg) return NULL;
    dsntp_agent_t *ag = (dsntp_agent_t *)calloc(1, sizeof(*ag));
    if (!ag) return NULL;
    ag->cfg = *cfg;
    dsntp_fsm_init(&ag->fsm);
    dsntp_clock_init(&ag->clock);
    dsntp_consensus_init(&ag->consensus, cfg->n, cfg->f);
    dsntp_window_init(&ag->window, cfg->window_L);
    ag->net = dsntp_net_open(cfg->port);
    ag->crypto = dsntp_crypto_create(cfg->privkey_path, cfg->pubkey_dir);
    ag->api = dsntp_api_local_start(cfg->sock_path, &ag->clock);
    ag->round = 0;
    return ag;
}

void dsntp_agent_destroy(dsntp_agent_t *ag) {
    if (!ag) return;
    dsntp_api_local_stop(ag->api);
    dsntp_crypto_destroy(ag->crypto);
    dsntp_net_close(ag->net);
    free(ag);
}

void dsntp_agent_request_stop(dsntp_agent_t *ag) {
    if (ag) ag->stop = 1;
}

static void sleep_ms(unsigned ms) {
#if defined(_WIN32)
    Sleep(ms);
#else
    usleep(ms * 1000u);
#endif
}

/*
 * 主闭环骨架（DRS §6.4）：
 * SyncWait → Collecting → Consensus → Estimating → Running → …
 * 具体 TIME_REQ 并发、ACK 收集、签名在此填空对接。
 */
dsntp_err_t dsntp_agent_run(dsntp_agent_t *ag) {
    if (!ag) return DSNTP_ERR_INVAL;
    dsntp_fsm_handle(&ag->fsm, DSNTP_EV_START);
    fprintf(stderr, "[dsntp] node=%u state=%s\n",
            (unsigned)ag->cfg.node_id, dsntp_fsm_state_name(dsntp_fsm_state(&ag->fsm)));

    while (!ag->stop) {
        dsntp_state_t st = dsntp_fsm_state(&ag->fsm);
        switch (st) {
        case DSNTP_ST_SYNC_WAIT:
            sleep_ms(ag->cfg.period_ms);
            dsntp_fsm_handle(&ag->fsm, DSNTP_EV_TIMER_TICK);
            ag->round++;
            break;
        case DSNTP_ST_COLLECTING:
            /* TODO FR-MEAS: 广播 TIME_REQ，收 TIME_RESP，超时 PeerFaultSuspect */
            dsntp_fsm_handle(&ag->fsm, DSNTP_EV_COLLECT_DONE);
            break;
        case DSNTP_ST_CONSENSUS:
            /* TODO FR-CNS: 中位数 + CONSENSUS_RESULT/ACK ≥2f+1 */
            dsntp_fsm_handle(&ag->fsm, DSNTP_EV_CONSENSUS_DONE);
            break;
        case DSNTP_ST_ESTIMATING:
            /* TODO FR-SYN: 窗口回归更新 clock */
            dsntp_fsm_handle(&ag->fsm, DSNTP_EV_ESTIMATE_DONE);
            break;
        case DSNTP_ST_RUNNING:
            dsntp_api_local_poll(ag->api);
            dsntp_fsm_handle(&ag->fsm, DSNTP_EV_TIMER_TICK);
            break;
        case DSNTP_ST_RECOVERING:
            /* TODO FR-FLT: 静默 → ANNOUNCE → ACK */
            dsntp_fsm_handle(&ag->fsm, DSNTP_EV_ALIGN_SYNC_DONE);
            break;
        default:
            sleep_ms(50);
            break;
        }
        fprintf(stderr, "[dsntp] round=%u state=%s synced=%llu\n",
                (unsigned)ag->round,
                dsntp_fsm_state_name(dsntp_fsm_state(&ag->fsm)),
                (unsigned long long)dsntp_clock_synced_ns(&ag->clock));
    }
    return DSNTP_OK;
}
