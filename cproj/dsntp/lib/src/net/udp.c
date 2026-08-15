#include "dsntp/net.h"
#include "dsntp/protocol.h"
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET dsntp_sock_t;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int dsntp_sock_t;
#define INVALID_SOCKET (-1)
#define closesocket close
#endif

struct dsntp_net {
    dsntp_sock_t fd;
    uint16_t     port;
};

dsntp_net_t *dsntp_net_open(uint16_t port) {
#if defined(_WIN32)
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return NULL;
#endif
    dsntp_net_t *n = (dsntp_net_t *)calloc(1, sizeof(*n));
    if (!n) return NULL;
    n->port = port ? port : (uint16_t)DSNTP_DEFAULT_PORT;
    n->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (n->fd == INVALID_SOCKET) { free(n); return NULL; }
    {
        int yes = 1;
        setsockopt(n->fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(n->port);
    if (bind(n->fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        closesocket(n->fd);
        free(n);
        return NULL;
    }
    return n;
}

void dsntp_net_close(dsntp_net_t *net) {
    if (!net) return;
    if (net->fd != INVALID_SOCKET) closesocket(net->fd);
    free(net);
#if defined(_WIN32)
    WSACleanup();
#endif
}

dsntp_err_t dsntp_net_sendto(dsntp_net_t *net, const char *host, uint16_t port,
                             const uint8_t *buf, size_t len) {
    if (!net || !host || !buf) return DSNTP_ERR_INVAL;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) return DSNTP_ERR_INVAL;
    int n = (int)sendto(net->fd, (const char *)buf, (int)len, 0,
                        (struct sockaddr *)&addr, sizeof(addr));
    return n < 0 ? DSNTP_ERR_IO : DSNTP_OK;
}

dsntp_err_t dsntp_net_recvfrom(dsntp_net_t *net, uint8_t *buf, size_t buf_len,
                               size_t *out_len, char *host, size_t host_len,
                               uint16_t *port, int timeout_ms) {
    if (!net || !buf || !out_len) return DSNTP_ERR_INVAL;
#if !defined(_WIN32)
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(net->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#else
    DWORD tv = (DWORD)timeout_ms;
    setsockopt(net->fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
#endif
    struct sockaddr_in peer;
    memset(&peer, 0, sizeof(peer));
#if defined(_WIN32)
    int peer_len = (int)sizeof(peer);
#else
    socklen_t peer_len = (socklen_t)sizeof(peer);
#endif
    int n = (int)recvfrom(net->fd, (char *)buf, (int)buf_len, 0,
                          (struct sockaddr *)&peer, &peer_len);
    if (n < 0) return DSNTP_ERR_TIMEOUT;
    *out_len = (size_t)n;
    if (port) *port = ntohs(peer.sin_port);
    if (host && host_len > 0) {
#if defined(_WIN32)
        const char *p = inet_ntop(AF_INET, &peer.sin_addr, host, host_len);
#else
        const char *p = inet_ntop(AF_INET, &peer.sin_addr, host, (socklen_t)host_len);
#endif
        if (!p) host[0] = '\0';
    }
    return DSNTP_OK;
}

dsntp_err_t dsntp_net_recv(dsntp_net_t *net, uint8_t *buf, size_t buf_len,
                           size_t *out_len, int timeout_ms) {
    return dsntp_net_recvfrom(net, buf, buf_len, out_len, NULL, 0, NULL, timeout_ms);
}
