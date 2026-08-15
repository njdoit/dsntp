/*
 * dsntp-time-shim — consume Agent UDS synced_ns, feed chrony SOCK (M5 stub)
 * IF-NTP / FR-NTP: gateway only; no external NTP sources.
 */
#include "dsntp/clock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#endif

static int read_synced_ns(const char *sock_path, unsigned long long *out) {
#if defined(_WIN32)
    (void)sock_path;
    *out = (unsigned long long)dsntp_clock_monotonic_ns();
    return 0;
#else
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path) - 1);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        close(fd);
        return -1;
    }
    char buf[64];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) return -1;
    buf[n] = '\0';
    *out = strtoull(buf, NULL, 10);
    return 0;
#endif
}

int main(int argc, char **argv) {
    const char *sock = "/tmp/time-agent.sock";
    const char *chrony_sock = "/var/run/chrony.dsntp.sock";
    int interval_ms = 1000;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--agent-sock") == 0 && i + 1 < argc) sock = argv[++i];
        else if (strcmp(argv[i], "--chrony-sock") == 0 && i + 1 < argc) chrony_sock = argv[++i];
        else if (strcmp(argv[i], "--interval-ms") == 0 && i + 1 < argc) interval_ms = atoi(argv[++i]);
    }

    fprintf(stderr, "dsntp-time-shim: agent=%s chrony=%s (scaffold)\n", sock, chrony_sock);

    for (;;) {
        unsigned long long synced = 0;
        if (read_synced_ns(sock, &synced) == 0) {
            /* TODO M5: write chrony SOCK reference-clock frame */
            fprintf(stderr, "[shim] synced_ns=%llu -> chrony SOCK (TODO)\n", synced);
        } else {
            fprintf(stderr, "[shim] agent UDS unavailable, retry\n");
        }
#if defined(_WIN32)
        Sleep((DWORD)interval_ms);
#else
        usleep((useconds_t)interval_ms * 1000u);
#endif
    }
    return 0;
}
