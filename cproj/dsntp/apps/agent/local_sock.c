#include "dsntp/api_local.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(_WIN32)
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif

struct dsntp_api_local {
    int fd;
    const dsntp_clock_t *clk;
    char path[256];
};

dsntp_api_local_t *dsntp_api_local_start(const char *sock_path, const dsntp_clock_t *clk) {
    dsntp_api_local_t *api = (dsntp_api_local_t *)calloc(1, sizeof(*api));
    if (!api) return NULL;
    api->clk = clk;
    api->fd = -1;
    if (sock_path) strncpy(api->path, sock_path, sizeof(api->path) - 1);
#if !defined(_WIN32)
    unlink(api->path);
    api->fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (api->fd >= 0) {
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, api->path, sizeof(addr.sun_path) - 1);
        if (bind(api->fd, (struct sockaddr *)&addr, sizeof(addr)) != 0 ||
            listen(api->fd, 4) != 0) {
            close(api->fd);
            api->fd = -1;
        } else {
            int flags = fcntl(api->fd, F_GETFL, 0);
            if (flags >= 0) fcntl(api->fd, F_SETFL, flags | O_NONBLOCK);
        }
    }
#else
    (void)clk;
#endif
    return api;
}

void dsntp_api_local_stop(dsntp_api_local_t *api) {
    if (!api) return;
#if !defined(_WIN32)
    if (api->fd >= 0) close(api->fd);
    unlink(api->path);
#endif
    free(api);
}

void dsntp_api_local_poll(dsntp_api_local_t *api) {
    if (!api || !api->clk) return;
#if !defined(_WIN32)
    if (api->fd < 0) return;
    for (;;) {
        int cfd = accept(api->fd, NULL, NULL);
        if (cfd < 0) break;
        dsntp_ns_t synced = dsntp_clock_synced_ns(api->clk);
        char buf[64];
        int n = snprintf(buf, sizeof(buf), "%llu\n", (unsigned long long)synced);
        if (n > 0) {
            ssize_t w = write(cfd, buf, (size_t)n);
            (void)w;
        }
        close(cfd);
    }
#else
    (void)dsntp_clock_synced_ns(api->clk);
#endif
}
