// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

// Front side: SHA-2 dispatch slots and OSSL_PARAM plumbing.
//
// Shared behavior is ordinary C at the family level. Each algorithm then has
// thin, typed wrappers naming its own backend entry points.

#include <assert.h>

#include "internal/backend.h"
#include "internal/backend/digests.h"
#include "internal/frontend/digests.h"
#include "internal/provider.h"

// Changes the DER OpenSSL emits for this digest inside PKI structures. The value
// must match the OpenSSL default provider's, or our encodings differ from it.
#define AWSLC_PROV_SHA2_FLAGS AWSLC_PROV_DIGEST_FLAG_ALGID_ABSENT

typedef int (*awslc_prov_sha2_init_fn)(void *ctx);
typedef int (*awslc_prov_sha2_update_fn)(void *ctx, const void *data,
                                         size_t len);
typedef int (*awslc_prov_sha2_final_fn)(void *ctx, unsigned char *out,
                                        size_t out_size);
typedef int (*awslc_prov_sha2_copy_fn)(void *dst, const void *src);

typedef struct {
  AWSLC_PROV_CTX *provctx;
  void *backend_ctx;
  size_t backend_ctx_size;
  const char *algorithm_name;
  int fips_approved;
} AWSLC_PROV_SHA2_CTX;

static void *awslc_prov_sha2_newctx(void *provctx, size_t backend_ctx_size,
                                    const char *algorithm_name) {
  AWSLC_PROV_SHA2_CTX *ctx = NULL;
  void *backend_ctx = NULL;
  AWSLC_PROV_CALL call;

  awslc_prov_call_begin(&call, provctx);
  ctx = awslc_prov_zalloc(sizeof(*ctx));
  if (ctx != NULL) {
    backend_ctx = awslc_prov_zalloc(backend_ctx_size);
    if (backend_ctx == NULL) {
      awslc_prov_clear_free(ctx, sizeof(*ctx));
      ctx = NULL;
    }
  }
  if (!AWSLC_PROV_CALL_END(&call, ctx != NULL, algorithm_name)) {
    return NULL;
  }

  ctx->backend_ctx = backend_ctx;
  ctx->provctx = (AWSLC_PROV_CTX *)provctx;
  ctx->backend_ctx_size = backend_ctx_size;
  ctx->algorithm_name = algorithm_name;
  ctx->fips_approved = awslc_prov_ctx_is_fips(ctx->provctx);
  return ctx;
}

static void awslc_prov_sha2_freectx(void *dctx) {
  AWSLC_PROV_SHA2_CTX *ctx = (AWSLC_PROV_SHA2_CTX *)dctx;

  if (ctx == NULL) {
    return;
  }
  awslc_prov_clear_free(ctx->backend_ctx, ctx->backend_ctx_size);
  awslc_prov_clear_free(ctx, sizeof(*ctx));
}

static void *awslc_prov_sha2_dupctx(void *dctx,
                                    awslc_prov_sha2_copy_fn copy) {
  AWSLC_PROV_SHA2_CTX *ctx = (AWSLC_PROV_SHA2_CTX *)dctx;
  AWSLC_PROV_SHA2_CTX *duplicate = NULL;

  if (ctx == NULL) {
    return NULL;
  }
  duplicate = awslc_prov_sha2_newctx(
      ctx->provctx, ctx->backend_ctx_size, ctx->algorithm_name);
  if (duplicate == NULL) {
    return NULL;
  }
  if (!copy(duplicate->backend_ctx, ctx->backend_ctx)) {
    awslc_prov_sha2_freectx(duplicate);
    return NULL;
  }
  duplicate->fips_approved = ctx->fips_approved;
  return duplicate;
}

static void awslc_prov_sha2_copyctx(void *outctx, void *inctx,
                                    awslc_prov_sha2_copy_fn copy) {
  AWSLC_PROV_SHA2_CTX *out = (AWSLC_PROV_SHA2_CTX *)outctx;
  AWSLC_PROV_SHA2_CTX *in = (AWSLC_PROV_SHA2_CTX *)inctx;

  if (out == NULL || in == NULL) {
    return;
  }
  // Unreachable: EVP calls copyctx only for two contexts of the same digest.
  assert(out->backend_ctx_size == in->backend_ctx_size);
  if (out->backend_ctx_size != in->backend_ctx_size) {
    return;
  }
  (void)copy(out->backend_ctx, in->backend_ctx);
  out->provctx = in->provctx;
  out->algorithm_name = in->algorithm_name;
  out->fips_approved = in->fips_approved;
}

// The init params are forwarded signature-operation params, not a digest-scoped
// array. No digest can honor keys such as pad-mode or saltlen, so ignore them.
static int awslc_prov_sha2_init_op(void *dctx, const OSSL_PARAM params[],
                                   awslc_prov_sha2_init_fn init) {
  AWSLC_PROV_SHA2_CTX *ctx = (AWSLC_PROV_SHA2_CTX *)dctx;
  AWSLC_PROV_CALL call;

  (void)params;
  if (ctx == NULL) {
    return 0;
  }
  ctx->fips_approved = awslc_prov_ctx_is_fips(ctx->provctx);

  awslc_prov_call_begin(&call, ctx->provctx);
  int ok = init(ctx->backend_ctx);
  return AWSLC_PROV_CALL_END(&call, ok, ctx->algorithm_name);
}

static int awslc_prov_sha2_update_op(void *dctx, const unsigned char *in,
                                     size_t inl,
                                     awslc_prov_sha2_update_fn update) {
  AWSLC_PROV_SHA2_CTX *ctx = (AWSLC_PROV_SHA2_CTX *)dctx;
  AWSLC_PROV_CALL call;

  if (ctx == NULL) {
    return 0;
  }
  // A zero-length update with a NULL |in| is legal per the EVP contract. AWS-LC
  // already treats len == 0 as a no-op, so this is a guard against a future
  // backend that does not, not a workaround for the current one.
  if (inl == 0) {
    return 1;
  }
  if (in == NULL) {
    AWSLC_PROV_ERROR_RAISE(ctx->provctx, AWSLC_PROV_R_INVALID_PARAMETER,
                           ctx->algorithm_name);
    return 0;
  }

  awslc_prov_call_begin(&call, ctx->provctx);
  int ok = update(ctx->backend_ctx, in, inl);
  return AWSLC_PROV_CALL_END(&call, ok, ctx->algorithm_name);
}

static int awslc_prov_sha2_final_op(void *dctx, unsigned char *out, size_t *outl,
                                    size_t outsz,
                                    awslc_prov_sha2_final_fn final_fn,
                                    size_t digest_size) {
  AWSLC_PROV_SHA2_CTX *ctx = (AWSLC_PROV_SHA2_CTX *)dctx;
  AWSLC_PROV_CALL call;

  if (ctx == NULL) {
    return 0;
  }
  if (out == NULL || outl == NULL) {
    AWSLC_PROV_ERROR_RAISE(ctx->provctx, AWSLC_PROV_R_INVALID_PARAMETER,
                           ctx->algorithm_name);
    return 0;
  }
  awslc_prov_call_begin(&call, ctx->provctx);
  int ok = final_fn(ctx->backend_ctx, out, outsz);
  if (!AWSLC_PROV_CALL_END_CHECK_APPROVED(
          &call, ok, ctx->algorithm_name,
          AWSLC_PROV_DIGEST_OPERATION_DESCRIPTION, &ctx->fips_approved)) {
    awslc_prov_cleanse(out, outsz < digest_size ? outsz : digest_size);
    return 0;
  }
  *outl = digest_size;
  return 1;
}

static int awslc_prov_sha2_get_ctx_params(void *dctx, OSSL_PARAM params[]) {
  AWSLC_PROV_SHA2_CTX *ctx = (AWSLC_PROV_SHA2_CTX *)dctx;

  if (ctx == NULL) {
    return 0;
  }
  return awslc_prov_digest_get_fips_indicator(ctx->provctx, params,
                                              ctx->fips_approved);
}

static int awslc_prov_sha2_get_params(OSSL_PARAM params[],
                                      size_t block_size, size_t digest_size) {
  return awslc_prov_digest_get_params(params, block_size, digest_size,
                                      AWSLC_PROV_SHA2_FLAGS);
}

// SHA-224

AWSLC_PROV_DECLARE_FIXED_DIGEST_SLOTS(sha224);

static void *awslc_prov_sha224_newctx(void *provctx) {
  return awslc_prov_sha2_newctx(provctx, awslc_prov_sha224_ctx_size(),
                                "SHA2-224");
}

static void awslc_prov_sha224_freectx(void *dctx) {
  awslc_prov_sha2_freectx(dctx);
}

static void *awslc_prov_sha224_dupctx(void *dctx) {
  return awslc_prov_sha2_dupctx(dctx, awslc_prov_sha224_copy);
}

static void awslc_prov_sha224_copyctx(void *outctx, void *inctx) {
  awslc_prov_sha2_copyctx(outctx, inctx, awslc_prov_sha224_copy);
}

static int awslc_prov_sha224_init_op(void *dctx, const OSSL_PARAM params[]) {
  return awslc_prov_sha2_init_op(dctx, params, awslc_prov_sha224_init);
}

static int awslc_prov_sha224_update_op(void *dctx, const unsigned char *in,
                                       size_t inl) {
  return awslc_prov_sha2_update_op(dctx, in, inl, awslc_prov_sha224_update);
}

static int awslc_prov_sha224_final_op(void *dctx, unsigned char *out,
                                      size_t *outl, size_t outsz) {
  return awslc_prov_sha2_final_op(dctx, out, outl, outsz,
                                  awslc_prov_sha224_final,
                                  awslc_prov_sha224_digest_size());
}

static int awslc_prov_sha224_get_params(OSSL_PARAM params[]) {
  return awslc_prov_sha2_get_params(params, awslc_prov_sha224_block_size(),
                                    awslc_prov_sha224_digest_size());
}

AWSLC_PROV_FIXED_DIGEST_DISPATCH_TABLE(sha224, sha2);

// SHA-256

AWSLC_PROV_DECLARE_FIXED_DIGEST_SLOTS(sha256);

static void *awslc_prov_sha256_newctx(void *provctx) {
  return awslc_prov_sha2_newctx(provctx, awslc_prov_sha256_ctx_size(),
                                "SHA2-256");
}

static void awslc_prov_sha256_freectx(void *dctx) {
  awslc_prov_sha2_freectx(dctx);
}

static void *awslc_prov_sha256_dupctx(void *dctx) {
  return awslc_prov_sha2_dupctx(dctx, awslc_prov_sha256_copy);
}

static void awslc_prov_sha256_copyctx(void *outctx, void *inctx) {
  awslc_prov_sha2_copyctx(outctx, inctx, awslc_prov_sha256_copy);
}

static int awslc_prov_sha256_init_op(void *dctx, const OSSL_PARAM params[]) {
  return awslc_prov_sha2_init_op(dctx, params, awslc_prov_sha256_init);
}

static int awslc_prov_sha256_update_op(void *dctx, const unsigned char *in,
                                       size_t inl) {
  return awslc_prov_sha2_update_op(dctx, in, inl, awslc_prov_sha256_update);
}

static int awslc_prov_sha256_final_op(void *dctx, unsigned char *out,
                                      size_t *outl, size_t outsz) {
  return awslc_prov_sha2_final_op(dctx, out, outl, outsz,
                                  awslc_prov_sha256_final,
                                  awslc_prov_sha256_digest_size());
}

static int awslc_prov_sha256_get_params(OSSL_PARAM params[]) {
  return awslc_prov_sha2_get_params(params, awslc_prov_sha256_block_size(),
                                    awslc_prov_sha256_digest_size());
}

AWSLC_PROV_FIXED_DIGEST_DISPATCH_TABLE(sha256, sha2);

// SHA-384

AWSLC_PROV_DECLARE_FIXED_DIGEST_SLOTS(sha384);

static void *awslc_prov_sha384_newctx(void *provctx) {
  return awslc_prov_sha2_newctx(provctx, awslc_prov_sha384_ctx_size(),
                                "SHA2-384");
}

static void awslc_prov_sha384_freectx(void *dctx) {
  awslc_prov_sha2_freectx(dctx);
}

static void *awslc_prov_sha384_dupctx(void *dctx) {
  return awslc_prov_sha2_dupctx(dctx, awslc_prov_sha384_copy);
}

static void awslc_prov_sha384_copyctx(void *outctx, void *inctx) {
  awslc_prov_sha2_copyctx(outctx, inctx, awslc_prov_sha384_copy);
}

static int awslc_prov_sha384_init_op(void *dctx, const OSSL_PARAM params[]) {
  return awslc_prov_sha2_init_op(dctx, params, awslc_prov_sha384_init);
}

static int awslc_prov_sha384_update_op(void *dctx, const unsigned char *in,
                                       size_t inl) {
  return awslc_prov_sha2_update_op(dctx, in, inl, awslc_prov_sha384_update);
}

static int awslc_prov_sha384_final_op(void *dctx, unsigned char *out,
                                      size_t *outl, size_t outsz) {
  return awslc_prov_sha2_final_op(dctx, out, outl, outsz,
                                  awslc_prov_sha384_final,
                                  awslc_prov_sha384_digest_size());
}

static int awslc_prov_sha384_get_params(OSSL_PARAM params[]) {
  return awslc_prov_sha2_get_params(params, awslc_prov_sha384_block_size(),
                                    awslc_prov_sha384_digest_size());
}

AWSLC_PROV_FIXED_DIGEST_DISPATCH_TABLE(sha384, sha2);

// SHA-512

AWSLC_PROV_DECLARE_FIXED_DIGEST_SLOTS(sha512);

static void *awslc_prov_sha512_newctx(void *provctx) {
  return awslc_prov_sha2_newctx(provctx, awslc_prov_sha512_ctx_size(),
                                "SHA2-512");
}

static void awslc_prov_sha512_freectx(void *dctx) {
  awslc_prov_sha2_freectx(dctx);
}

static void *awslc_prov_sha512_dupctx(void *dctx) {
  return awslc_prov_sha2_dupctx(dctx, awslc_prov_sha512_copy);
}

static void awslc_prov_sha512_copyctx(void *outctx, void *inctx) {
  awslc_prov_sha2_copyctx(outctx, inctx, awslc_prov_sha512_copy);
}

static int awslc_prov_sha512_init_op(void *dctx, const OSSL_PARAM params[]) {
  return awslc_prov_sha2_init_op(dctx, params, awslc_prov_sha512_init);
}

static int awslc_prov_sha512_update_op(void *dctx, const unsigned char *in,
                                       size_t inl) {
  return awslc_prov_sha2_update_op(dctx, in, inl, awslc_prov_sha512_update);
}

static int awslc_prov_sha512_final_op(void *dctx, unsigned char *out,
                                      size_t *outl, size_t outsz) {
  return awslc_prov_sha2_final_op(dctx, out, outl, outsz,
                                  awslc_prov_sha512_final,
                                  awslc_prov_sha512_digest_size());
}

static int awslc_prov_sha512_get_params(OSSL_PARAM params[]) {
  return awslc_prov_sha2_get_params(params, awslc_prov_sha512_block_size(),
                                    awslc_prov_sha512_digest_size());
}

AWSLC_PROV_FIXED_DIGEST_DISPATCH_TABLE(sha512, sha2);

// SHA-512/224

AWSLC_PROV_DECLARE_FIXED_DIGEST_SLOTS(sha512_224);

static void *awslc_prov_sha512_224_newctx(void *provctx) {
  return awslc_prov_sha2_newctx(provctx, awslc_prov_sha512_224_ctx_size(),
                                "SHA2-512/224");
}

static void awslc_prov_sha512_224_freectx(void *dctx) {
  awslc_prov_sha2_freectx(dctx);
}

static void *awslc_prov_sha512_224_dupctx(void *dctx) {
  return awslc_prov_sha2_dupctx(dctx, awslc_prov_sha512_224_copy);
}

static void awslc_prov_sha512_224_copyctx(void *outctx, void *inctx) {
  awslc_prov_sha2_copyctx(outctx, inctx, awslc_prov_sha512_224_copy);
}

static int awslc_prov_sha512_224_init_op(void *dctx,
                                         const OSSL_PARAM params[]) {
  return awslc_prov_sha2_init_op(dctx, params, awslc_prov_sha512_224_init);
}

static int awslc_prov_sha512_224_update_op(void *dctx,
                                           const unsigned char *in,
                                           size_t inl) {
  return awslc_prov_sha2_update_op(dctx, in, inl,
                                   awslc_prov_sha512_224_update);
}

static int awslc_prov_sha512_224_final_op(void *dctx, unsigned char *out,
                                          size_t *outl, size_t outsz) {
  return awslc_prov_sha2_final_op(dctx, out, outl, outsz,
                                  awslc_prov_sha512_224_final,
                                  awslc_prov_sha512_224_digest_size());
}

static int awslc_prov_sha512_224_get_params(OSSL_PARAM params[]) {
  return awslc_prov_sha2_get_params(params,
                                    awslc_prov_sha512_224_block_size(),
                                    awslc_prov_sha512_224_digest_size());
}

AWSLC_PROV_FIXED_DIGEST_DISPATCH_TABLE(sha512_224, sha2);

// SHA-512/256

AWSLC_PROV_DECLARE_FIXED_DIGEST_SLOTS(sha512_256);

static void *awslc_prov_sha512_256_newctx(void *provctx) {
  return awslc_prov_sha2_newctx(provctx, awslc_prov_sha512_256_ctx_size(),
                                "SHA2-512/256");
}

static void awslc_prov_sha512_256_freectx(void *dctx) {
  awslc_prov_sha2_freectx(dctx);
}

static void *awslc_prov_sha512_256_dupctx(void *dctx) {
  return awslc_prov_sha2_dupctx(dctx, awslc_prov_sha512_256_copy);
}

static void awslc_prov_sha512_256_copyctx(void *outctx, void *inctx) {
  awslc_prov_sha2_copyctx(outctx, inctx, awslc_prov_sha512_256_copy);
}

static int awslc_prov_sha512_256_init_op(void *dctx,
                                         const OSSL_PARAM params[]) {
  return awslc_prov_sha2_init_op(dctx, params, awslc_prov_sha512_256_init);
}

static int awslc_prov_sha512_256_update_op(void *dctx,
                                           const unsigned char *in,
                                           size_t inl) {
  return awslc_prov_sha2_update_op(dctx, in, inl,
                                   awslc_prov_sha512_256_update);
}

static int awslc_prov_sha512_256_final_op(void *dctx, unsigned char *out,
                                          size_t *outl, size_t outsz) {
  return awslc_prov_sha2_final_op(dctx, out, outl, outsz,
                                  awslc_prov_sha512_256_final,
                                  awslc_prov_sha512_256_digest_size());
}

static int awslc_prov_sha512_256_get_params(OSSL_PARAM params[]) {
  return awslc_prov_sha2_get_params(params,
                                    awslc_prov_sha512_256_block_size(),
                                    awslc_prov_sha512_256_digest_size());
}

AWSLC_PROV_FIXED_DIGEST_DISPATCH_TABLE(sha512_256, sha2);
