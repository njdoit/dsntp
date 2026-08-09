/*
 * types.h — 公共类型、错误码与角色（DRS §2 / DR-003）
 */
#ifndef DSNTP_TYPES_H
#define DSNTP_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum dsntp_role {
    DSNTP_ROLE_CONSENSUS_ONLY = 0,
    DSNTP_ROLE_GATEWAY        = 1
} dsntp_role_t;

typedef enum dsntp_err {
    DSNTP_OK              = 0,
    DSNTP_ERR_INVAL       = -1,
    DSNTP_ERR_IO          = -2,
    DSNTP_ERR_TIMEOUT     = -3,
    DSNTP_ERR_CRYPTO      = -4,
    DSNTP_ERR_PROTO       = -5,
    DSNTP_ERR_NOMEM       = -6,
    DSNTP_ERR_STATE       = -7,
    DSNTP_ERR_QUORUM      = -8
} dsntp_err_t;

typedef uint16_t dsntp_node_id_t;
typedef uint32_t dsntp_round_t;
typedef uint64_t dsntp_ns_t;

#ifdef __cplusplus
}
#endif

#endif /* DSNTP_TYPES_H */
