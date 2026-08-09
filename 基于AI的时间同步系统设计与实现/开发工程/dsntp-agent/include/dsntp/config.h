/*
 * config.h — 节点配置（DR-003/004）
 */
#ifndef DSNTP_CONFIG_H
#define DSNTP_CONFIG_H

#include "dsntp/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DSNTP_MAX_PEERS 32

typedef struct dsntp_config {
    dsntp_node_id_t node_id;
    dsntp_role_t    role;
    int             n;
    int             f;
    uint32_t        period_ms;
    uint32_t        dmax_ms;
    int             window_L;
    uint16_t        port;
    char            peers[DSNTP_MAX_PEERS][64];
    int             peer_count;
    char            privkey_path[256];
    char            pubkey_dir[256];
    int             require_crypto;
    int             verify_all;
    char            sock_path[256];
} dsntp_config_t;

void        dsntp_config_set_defaults(dsntp_config_t *cfg);
dsntp_err_t dsntp_config_load(dsntp_config_t *cfg, const char *path);

#ifdef __cplusplus
}
#endif

#endif /* DSNTP_CONFIG_H */
