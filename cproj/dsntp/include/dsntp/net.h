/*
 * net.h — UDP data plane (IF-TSYN, default 47500)
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
/* Same as recv, but also returns peer IPv4 host/port (needed to reply TIME_REQ). */
dsntp_err_t  dsntp_net_recvfrom(dsntp_net_t *net, uint8_t *buf, size_t buf_len,
                                size_t *out_len, char *host, size_t host_len,
                                uint16_t *port, int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* DSNTP_NET_H */
