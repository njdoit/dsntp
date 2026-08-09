/*
 * api_local.h — 本机合成时间接口（IF-APP）
 */
#ifndef DSNTP_API_LOCAL_H
#define DSNTP_API_LOCAL_H

#include "dsntp/types.h"
#include "dsntp/clock.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dsntp_api_local dsntp_api_local_t;

dsntp_api_local_t *dsntp_api_local_start(const char *sock_path, const dsntp_clock_t *clk);
void               dsntp_api_local_stop(dsntp_api_local_t *api);
void               dsntp_api_local_poll(dsntp_api_local_t *api);

#ifdef __cplusplus
}
#endif

#endif /* DSNTP_API_LOCAL_H */
