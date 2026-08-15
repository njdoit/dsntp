#include "dsntp/crypto.h"
#include <stdlib.h>
#include <string.h>

struct dsntp_crypto {
    int stub_ok; /* 1 = accept/produce zero sig (dev); 0 = fail crypto */
};

dsntp_crypto_t *dsntp_crypto_create(const char *priv_pem, const char *pub_dir) {
    (void)priv_pem;
    (void)pub_dir;
    dsntp_crypto_t *c = (dsntp_crypto_t *)calloc(1, sizeof(*c));
    if (c) {
#if defined(DSNTP_WITH_OPENSSL)
        c->stub_ok = 0; /* real OpenSSL path to be filled in M2 */
#else
        c->stub_ok = 1; /* scaffold: allow unsigned/zero-sig packets */
#endif
    }
    return c;
}

void dsntp_crypto_destroy(dsntp_crypto_t *ctx) {
    free(ctx);
}

dsntp_err_t dsntp_crypto_sign(dsntp_crypto_t *ctx,
                              const uint8_t *hdr_payload, size_t len,
                              uint8_t sig_out[DSNTP_SIGNATURE_SIZE]) {
    (void)hdr_payload;
    (void)len;
    if (!sig_out) return DSNTP_ERR_INVAL;
    memset(sig_out, 0, DSNTP_SIGNATURE_SIZE);
    if (!ctx || !ctx->stub_ok) return DSNTP_ERR_CRYPTO;
    return DSNTP_OK; /* TODO M2: OpenSSL ECDSA-P256 r||s */
}

dsntp_err_t dsntp_crypto_verify(dsntp_crypto_t *ctx, dsntp_node_id_t sender,
                                const uint8_t *hdr_payload, size_t len,
                                const uint8_t sig[DSNTP_SIGNATURE_SIZE]) {
    (void)sender;
    (void)hdr_payload;
    (void)len;
    (void)sig;
    if (!ctx || !ctx->stub_ok) return DSNTP_ERR_CRYPTO;
    return DSNTP_OK; /* TODO M2 */
}

bool dsntp_crypto_type_must_verify(uint8_t type) {
    return type == DSNTP_TYPE_START_SYNC ||
           type == DSNTP_TYPE_CONSENSUS_RESULT ||
           type == DSNTP_TYPE_FAULT_NOTIFY ||
           type == DSNTP_TYPE_RECOVER_ANNOUNCE;
}
