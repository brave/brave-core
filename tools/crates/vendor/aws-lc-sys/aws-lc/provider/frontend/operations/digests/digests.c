// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

// Digest-class parameter handling. The keys are common to the operation class;
// each family supplies the values and flags appropriate to its algorithms.

#include <openssl/core_names.h>
#include <openssl/params.h>

#include "internal/frontend/digests.h"

static const OSSL_PARAM awslc_prov_digest_gettable[] = {
    OSSL_PARAM_size_t(OSSL_DIGEST_PARAM_BLOCK_SIZE, NULL),
    OSSL_PARAM_size_t(OSSL_DIGEST_PARAM_SIZE, NULL),
    OSSL_PARAM_int(OSSL_DIGEST_PARAM_XOF, NULL),
    OSSL_PARAM_int(OSSL_DIGEST_PARAM_ALGID_ABSENT, NULL),
    OSSL_PARAM_END};

static const OSSL_PARAM awslc_prov_digest_gettable_ctx[] = {
    OSSL_PARAM_int(OSSL_ALG_PARAM_FIPS_APPROVED_INDICATOR, NULL),
    OSSL_PARAM_END};

const OSSL_PARAM *awslc_prov_digest_gettable_params(void *provctx) {
  (void)provctx;
  return awslc_prov_digest_gettable;
}

const OSSL_PARAM *awslc_prov_digest_gettable_ctx_params(void *dctx,
                                                        void *provctx) {
  (void)dctx;
  (void)provctx;
  return awslc_prov_digest_gettable_ctx;
}

int awslc_prov_digest_get_params(OSSL_PARAM params[], size_t block_size,
                                 size_t digest_size, uint32_t flags) {
  OSSL_PARAM *p = NULL;

  p = OSSL_PARAM_locate(params, OSSL_DIGEST_PARAM_BLOCK_SIZE);
  if (p != NULL && !OSSL_PARAM_set_size_t(p, block_size)) {
    return 0;
  }
  p = OSSL_PARAM_locate(params, OSSL_DIGEST_PARAM_SIZE);
  if (p != NULL && !OSSL_PARAM_set_size_t(p, digest_size)) {
    return 0;
  }
  p = OSSL_PARAM_locate(params, OSSL_DIGEST_PARAM_XOF);
  if (p != NULL &&
      !OSSL_PARAM_set_int(p, (flags & AWSLC_PROV_DIGEST_FLAG_XOF) != 0)) {
    return 0;
  }
  p = OSSL_PARAM_locate(params, OSSL_DIGEST_PARAM_ALGID_ABSENT);
  if (p != NULL &&
      !OSSL_PARAM_set_int(
          p, (flags & AWSLC_PROV_DIGEST_FLAG_ALGID_ABSENT) != 0)) {
    return 0;
  }
  return 1;
}

int awslc_prov_digest_get_fips_indicator(const AWSLC_PROV_CTX *provctx,
                                         OSSL_PARAM params[], int approved) {
  OSSL_PARAM *p =
      OSSL_PARAM_locate(params, OSSL_ALG_PARAM_FIPS_APPROVED_INDICATOR);

  if (p == NULL) {
    return 1;
  }
  if (!OSSL_PARAM_set_int(p, approved)) {
    AWSLC_PROV_ERROR_RAISE(provctx, AWSLC_PROV_R_INVALID_PARAMETER,
                           OSSL_ALG_PARAM_FIPS_APPROVED_INDICATOR);
    return 0;
  }
  return 1;
}
