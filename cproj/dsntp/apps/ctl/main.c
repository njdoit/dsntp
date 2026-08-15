/*
 * dsntp-ctl — minimal HTTP JSON control-plane stub (IF-CTL, port 8080)
 * Red line: must NOT participate in P2P consensus or rewrite Tc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET sock_t;
#define CLOSESOCK closesocket
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
typedef int sock_t;
#define INVALID_SOCKET (-1)
#define CLOSESOCK close
#endif

#define MAX_NODES 64
#define BODY_MAX  8192

typedef struct {
    int used;
    unsigned node_id;
    unsigned long long synced_ns;
    unsigned long long consensus_tc;
    unsigned round;
    char fsm[32];
} node_rec_t;

static node_rec_t g_nodes[MAX_NODES];
static int g_listen_port = 8080;

static void upsert_node(unsigned node_id, unsigned long long synced,
                        unsigned long long tc, unsigned round, const char *fsm) {
    for (int i = 0; i < MAX_NODES; i++) {
        if (g_nodes[i].used && g_nodes[i].node_id == node_id) {
            g_nodes[i].synced_ns = synced;
            g_nodes[i].consensus_tc = tc;
            g_nodes[i].round = round;
            if (fsm) strncpy(g_nodes[i].fsm, fsm, sizeof(g_nodes[i].fsm) - 1);
            return;
        }
    }
    for (int i = 0; i < MAX_NODES; i++) {
        if (!g_nodes[i].used) {
            g_nodes[i].used = 1;
            g_nodes[i].node_id = node_id;
            g_nodes[i].synced_ns = synced;
            g_nodes[i].consensus_tc = tc;
            g_nodes[i].round = round;
            if (fsm) strncpy(g_nodes[i].fsm, fsm, sizeof(g_nodes[i].fsm) - 1);
            return;
        }
    }
}

static void send_resp(sock_t c, int code, const char *ctype, const char *body) {
    char hdr[256];
    int blen = body ? (int)strlen(body) : 0;
    const char *reason = (code == 200) ? "OK" : (code == 404) ? "Not Found" : "Error";
    int n = snprintf(hdr, sizeof(hdr),
                     "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n",
                     code, reason, ctype, blen);
    send(c, hdr, n, 0);
    if (body && blen > 0) send(c, body, blen, 0);
}

static void handle(sock_t c) {
    char req[BODY_MAX];
    int n = recv(c, req, sizeof(req) - 1, 0);
    if (n <= 0) { CLOSESOCK(c); return; }
    req[n] = '\0';

    char method[16], path[256];
    if (sscanf(req, "%15s %255s", method, path) != 2) {
        send_resp(c, 400, "text/plain", "bad request");
        CLOSESOCK(c);
        return;
    }

    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/v1/health") == 0) {
        send_resp(c, 200, "application/json", "{\"ok\":true,\"service\":\"dsntp-ctl\"}");
    } else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/v1/nodes") == 0) {
        char body[BODY_MAX];
        size_t off = 0;
        off += (size_t)snprintf(body + off, sizeof(body) - off, "{\"nodes\":[");
        int first = 1;
        for (int i = 0; i < MAX_NODES; i++) {
            if (!g_nodes[i].used) continue;
            if (!first) off += (size_t)snprintf(body + off, sizeof(body) - off, ",");
            first = 0;
            off += (size_t)snprintf(body + off, sizeof(body) - off,
                "{\"node_id\":%u,\"synced_ns\":%llu,\"consensus_tc\":%llu,\"round\":%u,\"fsm_state\":\"%s\"}",
                g_nodes[i].node_id,
                (unsigned long long)g_nodes[i].synced_ns,
                (unsigned long long)g_nodes[i].consensus_tc,
                g_nodes[i].round,
                g_nodes[i].fsm[0] ? g_nodes[i].fsm : "unknown");
            if (off + 128 >= sizeof(body)) break;
        }
        snprintf(body + off, sizeof(body) - off, "]}");
        send_resp(c, 200, "application/json", body);
    } else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/v1/ingest/report") == 0) {
        char *body = strstr(req, "\r\n\r\n");
        unsigned node_id = 0, round = 0;
        unsigned long long synced = 0, tc = 0;
        char fsm[32] = {0};
        if (body) {
            body += 4;
            /* naive JSON field scrape */
            const char *p;
            if ((p = strstr(body, "\"node_id\""))) sscanf(p, "\"node_id\"%*[^0-9]%u", &node_id);
            if ((p = strstr(body, "\"synced_ns\""))) sscanf(p, "\"synced_ns\"%*[^0-9]%llu", &synced);
            if ((p = strstr(body, "\"consensus_tc\""))) sscanf(p, "\"consensus_tc\"%*[^0-9]%llu", &tc);
            if ((p = strstr(body, "\"round\""))) sscanf(p, "\"round\"%*[^0-9]%u", &round);
            if ((p = strstr(body, "\"fsm_state\""))) sscanf(p, "\"fsm_state\"%*[^A-Za-z]%31[A-Za-z_]", fsm);
        }
        if (node_id) upsert_node(node_id, synced, tc, round, fsm);
        send_resp(c, 200, "application/json", "{\"ok\":true}");
    } else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/v1/events") == 0) {
        send_resp(c, 200, "application/json", "{\"events\":[]}");
    } else {
        send_resp(c, 404, "application/json", "{\"error\":\"not found\"}");
    }
    CLOSESOCK(c);
}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) g_listen_port = atoi(argv[++i]);
    }

#if defined(_WIN32)
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;
#endif

    sock_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) return 1;
    int yes = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)g_listen_port);
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) != 0 || listen(s, 16) != 0) {
        fprintf(stderr, "bind/listen failed on %d\n", g_listen_port);
        CLOSESOCK(s);
        return 1;
    }

    fprintf(stderr, "dsntp-ctl listening on :%d (HTTP JSON stub)\n", g_listen_port);
    for (;;) {
        sock_t c = accept(s, NULL, NULL);
        if (c == INVALID_SOCKET) continue;
        handle(c);
    }
}
