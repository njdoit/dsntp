/*
 * net.h — UDP 数据面（IF-TSYN，默认 47500）
 */
#ifndef DSNTP_NET_H
#define DSNTP_NET_H

#include "dsntp/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dsntp_net dsntp_net_t;

dsntp_net_t *dsntp_net_open(uint16_t port);
void         dsntp_net_close(dsntp_net_t *net);

dsntp_err_t  dsntp_net_sendto(dsntp_net_t *net, const char *host, uint16_t port,
                              const uint8_t *buf, size_t len);
dsntp_err_t  dsntp_net_recv(dsntp_net_t *net, uint8_t *buf, size_t buf_len,
                            size_t *out_len, int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* DSNTP_NET_H */
