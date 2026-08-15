/* Soft stubs for TinyCC Windows builds without Winsock import libs */
#include "dsntp/net.h"
#include "dsntp/protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct dsntp_net {
    uint16_t port;
};

dsntp_net_t *dsntp_net_open(uint16_t port) {
    dsntp_net_t *n = (dsntp_net_t *)calloc(1, sizeof(*n));
    if (!n) return NULL;
    n->port = port ? port : (uint16_t)DSNTP_DEFAULT_PORT;
    fprintf(stderr, "[net-stub] open port %u (no real UDP)\n", (unsigned)n->port);
    return n;
}

void dsntp_net_close(dsntp_net_t *net) {
    free(net);
}

dsntp_err_t dsntp_net_sendto(dsntp_net_t *net, const char *host, uint16_t port,
                             const uint8_t *buf, size_t len) {
    (void)net; (void)host; (void)port; (void)buf; (void)len;
    return DSNTP_OK;
}

dsntp_err_t dsntp_net_recv(dsntp_net_t *net, uint8_t *buf, size_t buf_len,
                           size_t *out_len, int timeout_ms) {
    (void)net; (void)buf; (void)buf_len; (void)timeout_ms;
    if (out_len) *out_len = 0;
    return DSNTP_ERR_TIMEOUT;
}

dsntp_err_t dsntp_net_recvfrom(dsntp_net_t *net, uint8_t *buf, size_t buf_len,
                               size_t *out_len, char *host, size_t host_len,
                               uint16_t *port, int timeout_ms) {
    (void)host; (void)host_len; (void)port;
    return dsntp_net_recv(net, buf, buf_len, out_len, timeout_ms);
}
