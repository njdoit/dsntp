#include "dsntp/protocol.h"
#include <string.h>

static uint16_t be16(uint16_t v) {
    return (uint16_t)((v >> 8) | (v << 8));
}

static uint32_t be32(uint32_t v) {
    return ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8) |
           ((v & 0x00FF0000u) >> 8)  | ((v & 0xFF000000u) >> 24);
}

static uint64_t be64(uint64_t v) {
    return ((uint64_t)be32((uint32_t)(v & 0xffffffffu)) << 32) |
           (uint64_t)be32((uint32_t)(v >> 32));
}

double dsntp_alpha_from_wire(int64_t alpha_ppm) {
    return ((double)alpha_ppm / 1e6) * 1e-6;
}

int64_t dsntp_alpha_to_wire(double alpha) {
    return (int64_t)(alpha / 1e-6 * 1e6);
}

int dsntp_header_encode(const dsntp_header_t *h, uint8_t out[DSNTP_HEADER_SIZE]) {
    if (!h || !out) return DSNTP_ERR_INVAL;
    uint32_t magic = be32(h->magic);
    uint16_t sid = be16(h->sender_id);
    uint16_t plen = be16(h->payload_len);
    uint32_t round = be32(h->round);
    memcpy(out + 0, &magic, 4);
    out[4] = h->version;
    out[5] = h->type;
    out[6] = h->flags;
    out[7] = h->reserved;
    memcpy(out + 8, &sid, 2);
    memcpy(out + 10, &plen, 2);
    memcpy(out + 12, &round, 4);
    return DSNTP_OK;
}

int dsntp_header_decode(const uint8_t in[DSNTP_HEADER_SIZE], dsntp_header_t *h) {
    if (!in || !h) return DSNTP_ERR_INVAL;
    uint32_t magic, round;
    uint16_t sid, plen;
    memcpy(&magic, in + 0, 4);
    memcpy(&sid, in + 8, 2);
    memcpy(&plen, in + 10, 2);
    memcpy(&round, in + 12, 4);
    h->magic = be32(magic);
    h->version = in[4];
    h->type = in[5];
    h->flags = in[6];
    h->reserved = in[7];
    h->sender_id = be16(sid);
    h->payload_len = be16(plen);
    h->round = be32(round);
    if (h->magic != DSNTP_MAGIC || h->version != DSNTP_VERSION) return DSNTP_ERR_PROTO;
    return DSNTP_OK;
}

int dsntp_packet_encode(const dsntp_packet_t *pkt, uint8_t *buf, size_t buf_len, size_t *out_len) {
    if (!pkt || !buf || !out_len) return DSNTP_ERR_INVAL;
    size_t need = DSNTP_HEADER_SIZE + pkt->payload_len + DSNTP_SIGNATURE_SIZE;
    if (buf_len < need || pkt->payload_len > sizeof(pkt->payload)) return DSNTP_ERR_INVAL;
    dsntp_header_t h = pkt->hdr;
    h.payload_len = pkt->payload_len;
    if (dsntp_header_encode(&h, buf) != DSNTP_OK) return DSNTP_ERR_PROTO;
    memcpy(buf + DSNTP_HEADER_SIZE, pkt->payload, pkt->payload_len);
    memcpy(buf + DSNTP_HEADER_SIZE + pkt->payload_len, pkt->signature, DSNTP_SIGNATURE_SIZE);
    *out_len = need;
    return DSNTP_OK;
}

int dsntp_packet_decode(const uint8_t *buf, size_t len, dsntp_packet_t *pkt) {
    if (!buf || !pkt || len < DSNTP_HEADER_SIZE + DSNTP_SIGNATURE_SIZE) return DSNTP_ERR_INVAL;
    if (dsntp_header_decode(buf, &pkt->hdr) != DSNTP_OK) return DSNTP_ERR_PROTO;
    if (DSNTP_HEADER_SIZE + pkt->hdr.payload_len + DSNTP_SIGNATURE_SIZE != len) return DSNTP_ERR_PROTO;
    if (pkt->hdr.payload_len > sizeof(pkt->payload)) return DSNTP_ERR_PROTO;
    pkt->payload_len = pkt->hdr.payload_len;
    memcpy(pkt->payload, buf + DSNTP_HEADER_SIZE, pkt->payload_len);
    memcpy(pkt->signature, buf + DSNTP_HEADER_SIZE + pkt->payload_len, DSNTP_SIGNATURE_SIZE);
    return DSNTP_OK;
}

/* shared endian helpers for payload.c */
uint16_t dsntp_be16(uint16_t v);
uint32_t dsntp_be32(uint32_t v);
uint64_t dsntp_be64(uint64_t v);

uint16_t dsntp_be16(uint16_t v) { return be16(v); }
uint32_t dsntp_be32(uint32_t v) { return be32(v); }
uint64_t dsntp_be64(uint64_t v) { return be64(v); }
