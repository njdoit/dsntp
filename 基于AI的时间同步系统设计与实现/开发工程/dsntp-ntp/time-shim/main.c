/*
 * dsntp-time-shim — read Agent IF-APP UDS synced_ns and feed chrony SOCK.
 * Milestone: M5 skeleton. Does NOT join TSYN consensus / rewrite Tc.
 */
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#ifndef DSNTP_DEFAULT_AGENT_SOCK
#define DSNTP_DEFAULT_AGENT_SOCK "/var/run/time-agent.sock"
#endif

static void usage(const char *argv0) {
    fprintf(stderr,
            "Usage: %s [-s agent_uds] [-o chrony_sock] [-i interval_ms]\n"
            "  Reads synthetic time from Agent UDS and prints/feeds chrony.\n"
            "  Red line: no external NTP source; never rewrite consensus Tc.\n",
            argv0);
}

int main(int argc, char **argv) {
    const char *agent_sock = DSNTP_DEFAULT_AGENT_SOCK;
    const char *chrony_sock = "/var/run/chrony.dsntp.sock";
    int interval_ms = 200;
    int opt;

    while ((opt = getopt(argc, argv, "s:o:i:h")) != -1) {
        switch (opt) {
        case 's':
            agent_sock = optarg;
            break;
        case 'o':
            chrony_sock = optarg;
            break;
        case 'i':
            interval_ms = atoi(optarg);
            if (interval_ms < 50)
                interval_ms = 50;
            break;
        default:
            usage(argv[0]);
            return 2;
        }
    }

    fprintf(stderr,
            "dsntp-time-shim skeleton\n"
            "  agent_uds=%s\n"
            "  chrony_sock=%s (feed TODO: chrony SOCK refclock)\n"
            "  interval_ms=%d\n",
            agent_sock, chrony_sock, interval_ms);

    /* Skeleton loop: attempt UDS connect; real IF-APP framing lands in M5. */
    for (;;) {
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0) {
            perror("socket");
            sleep(1);
            continue;
        }
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, agent_sock, sizeof(addr.sun_path) - 1);
        if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
            char buf[128];
            ssize_t n = read(fd, buf, sizeof(buf) - 1);
            if (n > 0) {
                buf[n] = '\0';
                fprintf(stdout, "synced_ns_sample=%s\n", buf);
                fflush(stdout);
            }
        } else {
            /* Agent may not be up yet — keep retrying. */
        }
        close(fd);
        struct timespec ts;
        ts.tv_sec = interval_ms / 1000;
        ts.tv_nsec = (long)(interval_ms % 1000) * 1000000L;
        nanosleep(&ts, NULL);
    }
}
