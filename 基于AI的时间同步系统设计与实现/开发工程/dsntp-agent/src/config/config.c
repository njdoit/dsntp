#include "dsntp/config.h"
#include "dsntp/protocol.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void dsntp_config_set_defaults(dsntp_config_t *cfg) {
    if (!cfg) return;
    memset(cfg, 0, sizeof(*cfg));
    cfg->node_id = 1;
    cfg->role = DSNTP_ROLE_CONSENSUS_ONLY;
    cfg->n = 5;
    cfg->f = 1;
    cfg->period_ms = 1000;
    cfg->dmax_ms = 10;
    cfg->window_L = 10;
    cfg->port = DSNTP_DEFAULT_PORT;
    cfg->require_crypto = 0;
    cfg->verify_all = 1;
    strncpy(cfg->sock_path, "/var/run/time-agent.sock", sizeof(cfg->sock_path) - 1);
}

dsntp_err_t dsntp_config_load(dsntp_config_t *cfg, const char *path) {
    if (!cfg || !path) return DSNTP_ERR_INVAL;
    dsntp_config_set_defaults(cfg);
    FILE *fp = fopen(path, "r");
    if (!fp) return DSNTP_ERR_IO;
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\n' || *p == '\0') continue;
        char key[64], val[384];
        if (sscanf(p, "%63[^=]=%383s", key, val) != 2) continue;
        if (strcmp(key, "node_id") == 0) cfg->node_id = (dsntp_node_id_t)atoi(val);
        else if (strcmp(key, "n") == 0) cfg->n = atoi(val);
        else if (strcmp(key, "f") == 0) cfg->f = atoi(val);
        else if (strcmp(key, "period_ms") == 0) cfg->period_ms = (uint32_t)atoi(val);
        else if (strcmp(key, "dmax_ms") == 0) cfg->dmax_ms = (uint32_t)atoi(val);
        else if (strcmp(key, "window_L") == 0) cfg->window_L = atoi(val);
        else if (strcmp(key, "port") == 0) cfg->port = (uint16_t)atoi(val);
        else if (strcmp(key, "require_crypto") == 0) cfg->require_crypto = atoi(val);
        else if (strcmp(key, "verify_all") == 0) cfg->verify_all = atoi(val);
        else if (strcmp(key, "privkey_path") == 0) strncpy(cfg->privkey_path, val, sizeof(cfg->privkey_path) - 1);
        else if (strcmp(key, "pubkey_dir") == 0) strncpy(cfg->pubkey_dir, val, sizeof(cfg->pubkey_dir) - 1);
        else if (strcmp(key, "sock_path") == 0) strncpy(cfg->sock_path, val, sizeof(cfg->sock_path) - 1);
        else if (strcmp(key, "role") == 0) {
            cfg->role = (strcmp(val, "gateway") == 0) ? DSNTP_ROLE_GATEWAY : DSNTP_ROLE_CONSENSUS_ONLY;
        } else if (strcmp(key, "peer") == 0 && cfg->peer_count < DSNTP_MAX_PEERS) {
            strncpy(cfg->peers[cfg->peer_count++], val, 63);
        }
    }
    fclose(fp);
    return DSNTP_OK;
}
