// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

#ifndef AWSLC_PROVIDER_INTERNAL_FRONTEND_DIGESTS_H
#define AWSLC_PROVIDER_INTERNAL_FRONTEND_DIGESTS_H

// The frontend contract for the digest operation class: class-wide parameter
// helpers, fixed-length dispatch shapes, and the per-algorithm tables registry.c
// hands to the core.

#include <stddef.h>
#include <stdint.h>

#include <openssl/core_dispatch.h>

#include "internal/provider.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define AWSLC_PROV_DIGEST_OPERATION_DESCRIPTION "digest"

// Reported through OSSL_DIGEST_PARAM_XOF and OSSL_DIGEST_PARAM_ALGID_ABSENT.
#define AWSLC_PROV_DIGEST_FLAG_XOF 0x0001
#define AWSLC_PROV_DIGEST_FLAG_ALGID_ABSENT 0x0002

OSSL_FUNC_digest_gettable_params_fn awslc_prov_digest_gettable_params;
OSSL_FUNC_digest_gettable_ctx_params_fn
    awslc_prov_digest_gettable_ctx_params;

int awslc_prov_digest_get_params(OSSL_PARAM params[], size_t block_size,
                                 size_t digest_size, uint32_t flags);
int awslc_prov_digest_get_fips_indicator(const AWSLC_PROV_CTX *provctx,
                                         OSSL_PARAM params[], int approved);

// Declare one fixed-length digest's slots at the exact types OpenSSL calls.
#define AWSLC_PROV_DECLARE_FIXED_DIGEST_SLOTS(algorithm)                       \
  static OSSL_FUNC_digest_newctx_fn awslc_prov_##algorithm##_newctx;           \
  static OSSL_FUNC_digest_freectx_fn awslc_prov_##algorithm##_freectx;         \
  static OSSL_FUNC_digest_dupctx_fn awslc_prov_##algorithm##_dupctx;           \
  static OSSL_FUNC_digest_copyctx_fn awslc_prov_##algorithm##_copyctx;         \
  static OSSL_FUNC_digest_init_fn awslc_prov_##algorithm##_init_op;            \
  static OSSL_FUNC_digest_update_fn awslc_prov_##algorithm##_update_op;        \
  static OSSL_FUNC_digest_final_fn awslc_prov_##algorithm##_final_op;          \
  static OSSL_FUNC_digest_get_params_fn awslc_prov_##algorithm##_get_params

// Emit the table only after the ordinary C slot bodies above it are defined.
#define AWSLC_PROV_FIXED_DIGEST_DISPATCH_TABLE(algorithm, family)              \
  const OSSL_DISPATCH awslc_prov_##algorithm##_functions[] = {                 \
      {OSSL_FUNC_DIGEST_NEWCTX,                                                \
       (void (*)(void))awslc_prov_##algorithm##_newctx},                       \
      {OSSL_FUNC_DIGEST_INIT,                                                  \
       (void (*)(void))awslc_prov_##algorithm##_init_op},                      \
      {OSSL_FUNC_DIGEST_UPDATE,                                                \
       (void (*)(void))awslc_prov_##algorithm##_update_op},                    \
      {OSSL_FUNC_DIGEST_FINAL,                                                 \
       (void (*)(void))awslc_prov_##algorithm##_final_op},                     \
      {OSSL_FUNC_DIGEST_FREECTX,                                               \
       (void (*)(void))awslc_prov_##algorithm##_freectx},                      \
      {OSSL_FUNC_DIGEST_DUPCTX,                                                \
       (void (*)(void))awslc_prov_##algorithm##_dupctx},                       \
      {OSSL_FUNC_DIGEST_COPYCTX,                                               \
       (void (*)(void))awslc_prov_##algorithm##_copyctx},                      \
      {OSSL_FUNC_DIGEST_GET_PARAMS,                                            \
       (void (*)(void))awslc_prov_##algorithm##_get_params},                   \
      {OSSL_FUNC_DIGEST_GETTABLE_PARAMS,                                       \
       (void (*)(void))awslc_prov_digest_gettable_params},                     \
      {OSSL_FUNC_DIGEST_GET_CTX_PARAMS,                                        \
       (void (*)(void))awslc_prov_##family##_get_ctx_params},                  \
      {OSSL_FUNC_DIGEST_GETTABLE_CTX_PARAMS,                                   \
       (void (*)(void))awslc_prov_digest_gettable_ctx_params},                 \
      OSSL_DISPATCH_END}

#define AWSLC_PROV_DECLARE_DIGEST_TABLE(algorithm) \
  extern const OSSL_DISPATCH awslc_prov_##algorithm##_functions[]

// frontend/operations/digests/sha2.c
AWSLC_PROV_DECLARE_DIGEST_TABLE(sha224);
AWSLC_PROV_DECLARE_DIGEST_TABLE(sha256);
AWSLC_PROV_DECLARE_DIGEST_TABLE(sha384);
AWSLC_PROV_DECLARE_DIGEST_TABLE(sha512);
AWSLC_PROV_DECLARE_DIGEST_TABLE(sha512_224);
AWSLC_PROV_DECLARE_DIGEST_TABLE(sha512_256);

#if defined(__cplusplus)
}  // extern "C"
#endif

#endif  // AWSLC_PROVIDER_INTERNAL_FRONTEND_DIGESTS_H
