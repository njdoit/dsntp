/*
 * protocol.h — TSYN 线格式（DRS §7 / IF-TSYN / OC-01～03）
 * Header(16B) + Payload + Signature(64B)；FAULT_NOTIFY=0x06
 */
#ifndef DSNTP_PROTOCOL_H
#define DSNTP_PROTOCOL_H

#include "dsntp/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DSNTP_MAGIC           0x5453594Eu /* "TSYN" */
#define DSNTP_VERSION         0x02u
#define DSNTP_HEADER_SIZE     16
#define DSNTP_SIGNATURE_SIZE  64
#define DSNTP_MAX_PACKET_SIZE 512
#define DSNTP_DEFAULT_PORT    47500

#define DSNTP_TYPE_START_SYNC        0x01u
#define DSNTP_TYPE_TIME_REQ          0x02u
#define DSNTP_TYPE_TIME_RESP         0x03u
#define DSNTP_TYPE_CONSENSUS_RESULT  0x04u
#define DSNTP_TYPE_RECOVER_ANNOUNCE  0x05u
#define DSNTP_TYPE_FAULT_NOTIFY      0x06u
#define DSNTP_TYPE_CONSENSUS_ACK     0x07u
#define DSNTP_TYPE_RECOVER_ACK       0x08u

#define DSNTP_FLAG_URGENT            (1u << 1)

#define DSNTP_FAULT_REASON_TIMEOUT   0x01u
#define DSNTP_FAULT_REASON_OUTLIER   0x02u
#define DSNTP_FAULT_REASON_SIGNATURE 0x03u
#define DSNTP_FAULT_REASON_MANUAL    0x04u

#pragma pack(push, 1)
typedef struct dsntp_header {
    uint32_t magic;
    uint8_t  version;
    uint8_t  type;
    uint8_t  flags;
    uint8_t  reserved;
    uint16_t sender_id;
    uint16_t payload_len;
    uint32_t round;
} dsntp_header_t;

typedef struct dsntp_pl_start_sync {
    uint16_t leader_id;
    uint32_t period_ms;
    uint32_t dmax_ms;
    uint32_t start_round;
} dsntp_pl_start_sync_t;

typedef struct dsntp_pl_time_req {
    uint32_t request_id;
    uint64_t t0;
} dsntp_pl_time_req_t;

typedef struct dsntp_pl_time_resp {
    uint32_t request_id;
    uint16_t responder_id;
    uint16_t reserved;
    uint64_t t0;
    uint64_t t1;
    uint64_t t2;
} dsntp_pl_time_resp_t;

typedef struct dsntp_pl_consensus_result {
    uint64_t tc;
    uint16_t sample_count;
    uint16_t reserved;
} dsntp_pl_consensus_result_t;

typedef struct dsntp_pl_consensus_ack {
    uint64_t tc;
    uint8_t  agree;
    uint8_t  reserved[3];
} dsntp_pl_consensus_ack_t;

typedef struct dsntp_pl_fault_notify {
    uint16_t fault_node_id;
    uint8_t  reason;
    uint8_t  reserved;
    uint32_t detect_round;
} dsntp_pl_fault_notify_t;

typedef struct dsntp_pl_recover_announce {
    uint16_t node_id;
    uint16_t reserved;
    int64_t  beta_ns;
    int64_t  alpha_ppm; /* ppm × 1e6；解码见 dsntp_alpha_from_wire */
    uint64_t target_tc;
} dsntp_pl_recover_announce_t;

typedef struct dsntp_pl_recover_ack {
    uint16_t node_id;
    uint8_t  accept;
    uint8_t  reserved;
} dsntp_pl_recover_ack_t;
#pragma pack(pop)

typedef struct dsntp_packet {
    dsntp_header_t hdr;
    uint8_t        payload[DSNTP_MAX_PACKET_SIZE - DSNTP_HEADER_SIZE - DSNTP_SIGNATURE_SIZE];
    uint16_t       payload_len;
    uint8_t        signature[DSNTP_SIGNATURE_SIZE];
} dsntp_packet_t;

/* DR-001 / IF-TSYN-015 */
double   dsntp_alpha_from_wire(int64_t alpha_ppm);
int64_t  dsntp_alpha_to_wire(double alpha);

int dsntp_header_encode(const dsntp_header_t *h, uint8_t out[DSNTP_HEADER_SIZE]);
int dsntp_header_decode(const uint8_t in[DSNTP_HEADER_SIZE], dsntp_header_t *h);

int dsntp_packet_encode(const dsntp_packet_t *pkt, uint8_t *buf, size_t buf_len, size_t *out_len);
int dsntp_packet_decode(const uint8_t *buf, size_t len, dsntp_packet_t *pkt);

#ifdef __cplusplus
}
#endif

#endif /* DSNTP_PROTOCOL_H */
