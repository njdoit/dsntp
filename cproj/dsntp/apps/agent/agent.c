#include "dsntp/agent.h"
#include "dsntp/clock.h"
#include "dsntp/fsm.h"
#include "dsntp/consensus.h"
#include "dsntp/measure.h"
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

/* declared in reporter.c */
typedef struct dsntp_reporter dsntp_reporter_t;
dsntp_reporter_t *dsntp_reporter_start(const dsntp_config_t *cfg,
                                       const dsntp_clock_t *clk,
                                       const dsntp_fsm_t *fsm,
                                       const dsntp_ns_t *tc,
                                       const dsntp_round_t *round);
void dsntp_reporter_stop(dsntp_reporter_t *r);

#define DSNTP_COLLECT_DELTA_MS 2u
#define DSNTP_CONSENSUS_WAIT_MS 50

typedef struct {
    uint32_t req_id;
    char     host[64];
    uint16_t port;
    dsntp_ns_t t0;
    int      got;
    int      timed_out;
    dsntp_sample_t sample;
} dsntp_pending_req_t;

struct dsntp_agent {
    dsntp_config_t     cfg;
    dsntp_fsm_t        fsm;
    dsntp_clock_t      clock;
    dsntp_consensus_t  consensus;
    dsntp_window_t     window;
    dsntp_net_t       *net;
    dsntp_crypto_t    *crypto;
    dsntp_api_local_t *api;
    dsntp_reporter_t  *reporter;
    volatile int       stop;
    dsntp_round_t      round;
    uint32_t           next_req_id;
    /* per-round measurement table (FR-MEAS / FR-CNS) */
    dsntp_ns_t         samples[DSNTP_MAX_PEERS];
    int                sample_count;
    int64_t            thetas[DSNTP_MAX_PEERS];
    int                theta_count;
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
    ag->reporter = dsntp_reporter_start(&ag->cfg, &ag->clock, &ag->fsm,
                                        &ag->consensus.tc, &ag->round);
    ag->round = 0;
    ag->next_req_id = 1;
    return ag;
}

void dsntp_agent_destroy(dsntp_agent_t *ag) {
    if (!ag) return;
    dsntp_reporter_stop(ag->reporter);
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

static uint32_t collect_timeout_ms(const dsntp_config_t *cfg) {
    /* FR-MEAS-006: CollectTimeout = 2*Dmax + delta (delta=2ms) */
    return 2u * cfg->dmax_ms + DSNTP_COLLECT_DELTA_MS;
}

static int parse_peer(const char *s, char *host, size_t host_sz, uint16_t *port) {
    if (!s || !host || !port || host_sz == 0) return -1;
    const char *colon = strrchr(s, ':');
    if (!colon || colon == s) return -1;
    size_t hlen = (size_t)(colon - s);
    if (hlen >= host_sz) hlen = host_sz - 1;
    memcpy(host, s, hlen);
    host[hlen] = '\0';
    *port = (uint16_t)atoi(colon + 1);
    return (*port == 0) ? -1 : 0;
}

static dsntp_err_t agent_send_packet(dsntp_agent_t *ag, uint8_t type,
                                     const uint8_t *payload, size_t plen,
                                     const char *host, uint16_t port) {
    if (!ag || !ag->net || !payload || !host) return DSNTP_ERR_INVAL;
    dsntp_packet_t pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.hdr.magic = DSNTP_MAGIC;
    pkt.hdr.version = DSNTP_VERSION;
    pkt.hdr.type = type;
    pkt.hdr.flags = 0;
    pkt.hdr.sender_id = ag->cfg.node_id;
    pkt.hdr.round = ag->round;
    if (plen > sizeof(pkt.payload)) return DSNTP_ERR_INVAL;
    memcpy(pkt.payload, payload, plen);
    pkt.payload_len = (uint16_t)plen;

    uint8_t wire[DSNTP_MAX_PACKET_SIZE];
    size_t wire_len = 0;
    if (dsntp_packet_encode(&pkt, wire, sizeof(wire), &wire_len) != DSNTP_OK)
        return DSNTP_ERR_PROTO;
    /* Sign Header||Payload (signature area still zero until filled). */
    if (dsntp_crypto_sign(ag->crypto, wire, DSNTP_HEADER_SIZE + plen,
                          pkt.signature) != DSNTP_OK &&
        ag->cfg.require_crypto) {
        return DSNTP_ERR_CRYPTO;
    }
    memcpy(wire + DSNTP_HEADER_SIZE + plen, pkt.signature, DSNTP_SIGNATURE_SIZE);
    return dsntp_net_sendto(ag->net, host, port, wire, wire_len);
}

static dsntp_err_t reply_time_resp(dsntp_agent_t *ag, const dsntp_packet_t *req_pkt,
                                   const dsntp_pl_time_req_t *req,
                                   const char *host, uint16_t port,
                                   dsntp_ns_t t1) {
    dsntp_pl_time_resp_t resp;
    memset(&resp, 0, sizeof(resp));
    resp.request_id = req->request_id;
    resp.responder_id = ag->cfg.node_id;
    resp.t0 = req->t0;
    resp.t1 = t1;
    resp.t2 = dsntp_clock_monotonic_ns(); /* FR-MEAS-002: fill T2 immediately */
    uint8_t pl[64];
    size_t plen = 0;
    if (dsntp_pl_time_resp_encode(&resp, pl, sizeof(pl), &plen) != DSNTP_OK)
        return DSNTP_ERR_PROTO;
    (void)req_pkt;
    return agent_send_packet(ag, DSNTP_TYPE_TIME_RESP, pl, plen, host, port);
}

static dsntp_err_t reply_consensus_ack(dsntp_agent_t *ag,
                                       const dsntp_pl_consensus_result_t *res,
                                       const char *host, uint16_t port) {
    dsntp_pl_consensus_ack_t ack;
    memset(&ack, 0, sizeof(ack));
    ack.tc = res->tc;
    /* Agree when we hold a valid Tc (own or previously accepted). */
    ack.agree = ag->consensus.tc_valid ? 1 : 0;
    uint8_t pl[32];
    size_t plen = 0;
    if (dsntp_pl_consensus_ack_encode(&ack, pl, sizeof(pl), &plen) != DSNTP_OK)
        return DSNTP_ERR_PROTO;
    return agent_send_packet(ag, DSNTP_TYPE_CONSENSUS_ACK, pl, plen, host, port);
}

/*
 * Process one inbound packet. Used in Collecting/Consensus/Running so peers
 * can always get TIME_RESP during our Running window.
 * pending/n_pending: optional outstanding TIME_REQ table (Collecting only).
 * ack_agree: optional counter for CONSENSUS_ACK (Consensus only).
 */
static void agent_handle_packet(dsntp_agent_t *ag, const uint8_t *buf, size_t len,
                                const char *host, uint16_t port,
                                dsntp_pending_req_t *pending, int n_pending,
                                int *ack_agree) {
    dsntp_packet_t pkt;
    if (dsntp_packet_decode(buf, len, &pkt) != DSNTP_OK) return;

    if (ag->cfg.verify_all || dsntp_crypto_type_must_verify(pkt.hdr.type)) {
        if (dsntp_crypto_verify(ag->crypto, pkt.hdr.sender_id, buf,
                                DSNTP_HEADER_SIZE + pkt.payload_len,
                                pkt.signature) != DSNTP_OK) {
            return;
        }
    }

    switch (pkt.hdr.type) {
    case DSNTP_TYPE_TIME_REQ: {
        dsntp_pl_time_req_t req;
        if (dsntp_pl_time_req_decode(pkt.payload, pkt.payload_len, &req) != DSNTP_OK)
            return;
        dsntp_ns_t t1 = dsntp_clock_monotonic_ns();
        (void)reply_time_resp(ag, &pkt, &req, host, port, t1);
        break;
    }
    case DSNTP_TYPE_TIME_RESP: {
        if (!pending || n_pending <= 0) break;
        dsntp_pl_time_resp_t resp;
        if (dsntp_pl_time_resp_decode(pkt.payload, pkt.payload_len, &resp) != DSNTP_OK)
            break;
        dsntp_ns_t t3 = dsntp_clock_monotonic_ns();
        dsntp_ns_t dmax_ns = (dsntp_ns_t)ag->cfg.dmax_ms * 1000000ull;
        for (int i = 0; i < n_pending; i++) {
            if (pending[i].got || pending[i].req_id != resp.request_id) continue;
            pending[i].sample.t0 = resp.t0;
            pending[i].sample.t1 = resp.t1;
            pending[i].sample.t2 = resp.t2;
            pending[i].sample.t3 = t3;
            if (dsntp_measure_compute(&pending[i].sample, dmax_ns) == DSNTP_OK &&
                pending[i].sample.valid) {
                pending[i].got = 1;
            }
            break;
        }
        break;
    }
    case DSNTP_TYPE_CONSENSUS_RESULT: {
        dsntp_pl_consensus_result_t res;
        if (dsntp_pl_consensus_result_decode(pkt.payload, pkt.payload_len, &res) != DSNTP_OK)
            break;
        (void)reply_consensus_ack(ag, &res, host, port);
        break;
    }
    case DSNTP_TYPE_CONSENSUS_ACK: {
        if (!ack_agree) break;
        dsntp_pl_consensus_ack_t ack;
        if (dsntp_pl_consensus_ack_decode(pkt.payload, pkt.payload_len, &ack) != DSNTP_OK)
            break;
        if (ack.agree) (*ack_agree)++;
        break;
    }
    default:
        break;
    }
}

static void agent_poll_once(dsntp_agent_t *ag, int timeout_ms,
                            dsntp_pending_req_t *pending, int n_pending,
                            int *ack_agree) {
    if (!ag->net) return;
    uint8_t buf[DSNTP_MAX_PACKET_SIZE];
    size_t n = 0;
    char host[64];
    uint16_t port = 0;
    /* First wait up to timeout_ms, then drain any queued datagrams (1ms). */
    int wait = timeout_ms > 0 ? timeout_ms : 1;
    for (;;) {
        if (dsntp_net_recvfrom(ag->net, buf, sizeof(buf), &n, host, sizeof(host),
                              &port, wait) != DSNTP_OK)
            return;
        agent_handle_packet(ag, buf, n, host, port, pending, n_pending, ack_agree);
        wait = 1; /* drain remainder without burning the collect budget */
    }
}

/* FR-MEAS: broadcast TIME_REQ, gather TIME_RESP within CollectTimeout. */
static void agent_do_collecting(dsntp_agent_t *ag) {
    ag->sample_count = 0;
    ag->theta_count = 0;

    dsntp_pending_req_t pending[DSNTP_MAX_PEERS];
    memset(pending, 0, sizeof(pending));
    int n_pending = 0;

    for (int i = 0; i < ag->cfg.peer_count && n_pending < DSNTP_MAX_PEERS; i++) {
        char host[64];
        uint16_t port = 0;
        if (parse_peer(ag->cfg.peers[i], host, sizeof(host), &port) != 0) continue;

        dsntp_pl_time_req_t req;
        memset(&req, 0, sizeof(req));
        req.request_id = ag->next_req_id++;
        req.t0 = dsntp_clock_monotonic_ns();

        uint8_t pl[32];
        size_t plen = 0;
        if (dsntp_pl_time_req_encode(&req, pl, sizeof(pl), &plen) != DSNTP_OK) continue;
        if (agent_send_packet(ag, DSNTP_TYPE_TIME_REQ, pl, plen, host, port) != DSNTP_OK)
            continue;

        pending[n_pending].req_id = req.request_id;
        strncpy(pending[n_pending].host, host, sizeof(pending[n_pending].host) - 1);
        pending[n_pending].port = port;
        pending[n_pending].t0 = req.t0;
        n_pending++;
    }

    /* Include local clock as one sample so n=1 lab runs can still form Tc. */
    if (ag->sample_count < DSNTP_MAX_PEERS) {
        ag->samples[ag->sample_count++] = dsntp_clock_monotonic_ns();
    }

    uint32_t budget = collect_timeout_ms(&ag->cfg);
    dsntp_ns_t t_end = dsntp_clock_monotonic_ns() + (dsntp_ns_t)budget * 1000000ull;
    while (!ag->stop && dsntp_clock_monotonic_ns() < t_end) {
        int all_got = (n_pending > 0);
        for (int i = 0; i < n_pending; i++) {
            if (!pending[i].got) { all_got = 0; break; }
        }
        if (n_pending > 0 && all_got) break;

        int remain_ms = (int)((t_end - dsntp_clock_monotonic_ns()) / 1000000ull);
        if (remain_ms <= 0) break;
        if (remain_ms > 5) remain_ms = 5;
        agent_poll_once(ag, remain_ms, pending, n_pending, NULL);
    }

    for (int i = 0; i < n_pending; i++) {
        if (pending[i].got && pending[i].sample.valid) {
            /* Candidate common time ≈ local_mid + theta = (T1+T2)/2 */
            dsntp_ns_t cand = (pending[i].sample.t1 + pending[i].sample.t2) / 2;
            if (ag->sample_count < DSNTP_MAX_PEERS)
                ag->samples[ag->sample_count++] = cand;
            if (ag->theta_count < DSNTP_MAX_PEERS)
                ag->thetas[ag->theta_count++] = pending[i].sample.theta_ns;
        } else {
            /* FR-MEAS-006: timeout → PeerFaultSuspect; local FSM unchanged */
            pending[i].timed_out = 1;
            dsntp_fsm_handle(&ag->fsm, DSNTP_EV_PEER_FAULT_SUSPECT);
            fprintf(stderr, "[dsntp-agent] peer timeout %s:%u (suspect)\n",
                    pending[i].host, (unsigned)pending[i].port);
        }
    }

    dsntp_fsm_handle(&ag->fsm, DSNTP_EV_COLLECT_DONE);
}

/* FR-CNS: median Tc + CONSENSUS_RESULT/ACK quorum (>=2f+1). */
static void agent_do_consensus(dsntp_agent_t *ag) {
    dsntp_err_t med = dsntp_consensus_median(&ag->consensus, ag->samples, ag->sample_count);
    if (med == DSNTP_ERR_QUORUM) {
        fprintf(stderr, "[dsntp-agent] samples=%d < 2f+1; keep previous Tc\n",
                ag->sample_count);
    }

    int ack_agree = 0;
    if (ag->consensus.tc_valid) {
        /* Count self toward ACK quorum after publishing RESULT. */
        ack_agree = 1;
        dsntp_pl_consensus_result_t res;
        memset(&res, 0, sizeof(res));
        res.tc = ag->consensus.tc;
        res.sample_count = (uint16_t)ag->sample_count;
        uint8_t pl[32];
        size_t plen = 0;
        if (dsntp_pl_consensus_result_encode(&res, pl, sizeof(pl), &plen) == DSNTP_OK) {
            for (int i = 0; i < ag->cfg.peer_count; i++) {
                char host[64];
                uint16_t port = 0;
                if (parse_peer(ag->cfg.peers[i], host, sizeof(host), &port) != 0) continue;
                (void)agent_send_packet(ag, DSNTP_TYPE_CONSENSUS_RESULT, pl, plen, host, port);
            }
        }

        dsntp_ns_t t_end = dsntp_clock_monotonic_ns() +
                           (dsntp_ns_t)DSNTP_CONSENSUS_WAIT_MS * 1000000ull;
        while (!ag->stop && dsntp_clock_monotonic_ns() < t_end) {
            if (dsntp_consensus_ack_quorum(&ag->consensus, ack_agree)) break;
            int remain_ms = (int)((t_end - dsntp_clock_monotonic_ns()) / 1000000ull);
            if (remain_ms <= 0) break;
            if (remain_ms > 5) remain_ms = 5;
            agent_poll_once(ag, remain_ms, NULL, 0, &ack_agree);
        }
        if (!dsntp_consensus_ack_quorum(&ag->consensus, ack_agree)) {
            fprintf(stderr, "[dsntp-agent] consensus ACK quorum not met (agree=%d)\n",
                    ack_agree);
        }
    } else {
        /* Still drain TIME_REQ from peers briefly. */
        for (int i = 0; i < 5 && !ag->stop; i++)
            agent_poll_once(ag, 2, NULL, 0, NULL);
    }

    dsntp_fsm_handle(&ag->fsm, DSNTP_EV_CONSENSUS_DONE);
}

/* FR-SYN: window regression → clock estimates; sync_c anchors at Tc when valid. */
static void agent_do_estimating(dsntp_agent_t *ag) {
    for (int i = 0; i < ag->theta_count; i++)
        dsntp_window_push(&ag->window, ag->thetas[i]);

    double alpha = 0.0;
    int64_t beta = 0;
    if (dsntp_window_regress(&ag->window, &alpha, &beta) == DSNTP_OK) {
        /* Anchor on local monotonic at estimate time (Tc stays in consensus). */
        dsntp_clock_update_estimates(&ag->clock, alpha, beta,
                                     dsntp_clock_monotonic_ns());
    }
    dsntp_fsm_handle(&ag->fsm, DSNTP_EV_ESTIMATE_DONE);
}

/*
 * Main loop (DRS §6.4):
 * SyncWait -> Collecting -> Consensus -> Estimating -> Running -> ...
 */
dsntp_err_t dsntp_agent_run(dsntp_agent_t *ag) {
    if (!ag) return DSNTP_ERR_INVAL;
    if (!ag->net) {
        fprintf(stderr, "[dsntp-agent] UDP open failed (port %u)\n",
                (unsigned)ag->cfg.port);
        return DSNTP_ERR_IO;
    }
    dsntp_fsm_handle(&ag->fsm, DSNTP_EV_START);
    fprintf(stderr, "[dsntp-agent] node=%u state=%s port=%u peers=%d\n",
            (unsigned)ag->cfg.node_id,
            dsntp_fsm_state_name(dsntp_fsm_state(&ag->fsm)),
            (unsigned)ag->cfg.port,
            ag->cfg.peer_count);

    while (!ag->stop) {
        dsntp_state_t st = dsntp_fsm_state(&ag->fsm);
        switch (st) {
        case DSNTP_ST_SYNC_WAIT:
            /* Answer peer TIME_REQ while waiting for period. */
            {
                dsntp_ns_t t_end = dsntp_clock_monotonic_ns() +
                                   (dsntp_ns_t)ag->cfg.period_ms * 1000000ull;
                while (!ag->stop && dsntp_clock_monotonic_ns() < t_end) {
                    int remain = (int)((t_end - dsntp_clock_monotonic_ns()) / 1000000ull);
                    if (remain <= 0) break;
                    if (remain > 20) remain = 20;
                    agent_poll_once(ag, remain, NULL, 0, NULL);
                }
            }
            dsntp_fsm_handle(&ag->fsm, DSNTP_EV_TIMER_TICK);
            ag->round++;
            break;

        case DSNTP_ST_COLLECTING:
            agent_do_collecting(ag);
            break;

        case DSNTP_ST_CONSENSUS:
            agent_do_consensus(ag);
            break;

        case DSNTP_ST_ESTIMATING:
            agent_do_estimating(ag);
            break;

        case DSNTP_ST_RUNNING:
            dsntp_api_local_poll(ag->api);
            agent_poll_once(ag, 20, NULL, 0, NULL);
            dsntp_fsm_handle(&ag->fsm, DSNTP_EV_TIMER_TICK);
            break;

        case DSNTP_ST_RECOVERING:
            /* TODO M2 FR-FLT: silent + ANNOUNCE/ACK */
            dsntp_fsm_handle(&ag->fsm, DSNTP_EV_ALIGN_SYNC_DONE);
            break;

        default:
            sleep_ms(50);
            break;
        }

        fprintf(stderr, "[dsntp-agent] round=%u state=%s tc=%llu synced=%llu samples=%d\n",
                (unsigned)ag->round,
                dsntp_fsm_state_name(dsntp_fsm_state(&ag->fsm)),
                (unsigned long long)ag->consensus.tc,
                (unsigned long long)dsntp_clock_synced_ns(&ag->clock),
                ag->sample_count);
    }
    return DSNTP_OK;
}
