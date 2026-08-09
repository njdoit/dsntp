#include "dsntp/agent.h"
#include "dsntp/config.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    const char *conf = "deploy/agent.example.conf";
    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) && i + 1 < argc) {
            conf = argv[++i];
        }
    }

    dsntp_config_t cfg;
    if (dsntp_config_load(&cfg, conf) != DSNTP_OK) {
        fprintf(stderr, "load config failed: %s (using defaults)\n", conf);
        dsntp_config_set_defaults(&cfg);
    }

    dsntp_agent_t *ag = dsntp_agent_create(&cfg);
    if (!ag) {
        fprintf(stderr, "agent create failed\n");
        return 1;
    }

    fprintf(stderr, "dsntp-agent starting (DRS-001 skeleton)\n");
    dsntp_err_t rc = dsntp_agent_run(ag);
    dsntp_agent_destroy(ag);
    return rc == DSNTP_OK ? 0 : 1;
}
