/*
 * agent.h — main loop assembly (DRS §5.10 / §6.4)
 */
#ifndef DSNTP_AGENT_H
#define DSNTP_AGENT_H

#include "dsntp/config.h"
#include "dsntp/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dsntp_agent dsntp_agent_t;

dsntp_agent_t *dsntp_agent_create(const dsntp_config_t *cfg);
void           dsntp_agent_destroy(dsntp_agent_t *ag);
dsntp_err_t    dsntp_agent_run(dsntp_agent_t *ag); /* blocking main loop */
void           dsntp_agent_request_stop(dsntp_agent_t *ag);

#ifdef __cplusplus
}
#endif

#endif /* DSNTP_AGENT_H */
