#include "dsntp/consensus.h"
#include <stdlib.h>
#include <string.h>

void dsntp_consensus_init(dsntp_consensus_t *c, int n, int f) {
    if (!c) return;
    c->n = n;
    c->f = f;
    c->tc = 0;
    c->tc_valid = false;
}

static int cmp_ns(const void *a, const void *b) {
    dsntp_ns_t x = *(const dsntp_ns_t *)a;
    dsntp_ns_t y = *(const dsntp_ns_t *)b;
    return (x > y) - (x < y);
}

bool dsntp_consensus_quorum_ok(const dsntp_consensus_t *c, int sample_count) {
    if (!c) return false;
    return sample_count >= (2 * c->f + 1);
}

bool dsntp_consensus_ack_quorum(const dsntp_consensus_t *c, int agree_count) {
    if (!c) return false;
    return agree_count >= (2 * c->f + 1);
}

dsntp_err_t dsntp_consensus_median(dsntp_consensus_t *c, const dsntp_ns_t *samples, int count) {
    if (!c || !samples || count <= 0) return DSNTP_ERR_INVAL;
    if (!dsntp_consensus_quorum_ok(c, count)) {
        /* FR-CNS-005：不足 2f+1 不更新 Tc */
        return DSNTP_ERR_QUORUM;
    }
    dsntp_ns_t *tmp = (dsntp_ns_t *)malloc((size_t)count * sizeof(dsntp_ns_t));
    if (!tmp) return DSNTP_ERR_NOMEM;
    memcpy(tmp, samples, (size_t)count * sizeof(dsntp_ns_t));
    qsort(tmp, (size_t)count, sizeof(dsntp_ns_t), cmp_ns);
    c->tc = tmp[count / 2];
    c->tc_valid = true;
    free(tmp);
    return DSNTP_OK;
}
