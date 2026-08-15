/*
 * crypto.h — ECDSA-P256 signatures (FR-SEC); Signature = 64B r||s
 */
#ifndef DSNTP_CRYPTO_H
#define DSNTP_CRYPTO_H

#include "dsntp/types.h"
#include "dsntp/protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dsntp_crypto dsntp_crypto_t;

dsntp_crypto_t *dsntp_crypto_create(const char *priv_pem, const char *pub_dir);
void            dsntp_crypto_destroy(dsntp_crypto_t *ctx);

/* Sign input: Header || Payload */
dsntp_err_t dsntp_crypto_sign(dsntp_crypto_t *ctx,
                              const uint8_t *hdr_payload, size_t len,
                              uint8_t sig_out[DSNTP_SIGNATURE_SIZE]);

dsntp_err_t dsntp_crypto_verify(dsntp_crypto_t *ctx, dsntp_node_id_t sender,
                                const uint8_t *hdr_payload, size_t len,
                                const uint8_t sig[DSNTP_SIGNATURE_SIZE]);

bool dsntp_crypto_type_must_verify(uint8_t type);

#ifdef __cplusplus
}
#endif

#endif /* DSNTP_CRYPTO_H */
