// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

#ifndef AWSLC_PROVIDER_INTERNAL_BACKEND_DIGESTS_H
#define AWSLC_PROVIDER_INTERNAL_BACKEND_DIGESTS_H

// The digest entry points the back side implements, one set per algorithm.
//
// This header crosses the boundary, so it obeys the rule ../backend.h states:
// plain C types only. Read that file first; it documents the whole arrangement.
//
// Every algorithm here has the same shape, which is what lets the front side's
// slots be thin:
//
//   ctx_size()               bytes the caller must allocate for the context
//   digest_size()            the digest length
//   block_size()             the input block length
//   init(ctx)                initialize a caller-allocated context
//   update(ctx, data, len)   absorb input
//   final(ctx, out, size)    write the digest, rejecting an undersized |out|
//   copy(dst, src)           duplicate context state
//
// Returning 1 on success and 0 on failure, per ../backend.h.

#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

// Declarations only. Backend implementations remain ordinary C so their AWS-LC
// types, constants, and function bindings stay visible to review.
#define AWSLC_PROV_DECLARE_DIGEST_BACKEND(algorithm)               \
  size_t awslc_prov_##algorithm##_ctx_size(void);                  \
  size_t awslc_prov_##algorithm##_digest_size(void);               \
  size_t awslc_prov_##algorithm##_block_size(void);                \
  int awslc_prov_##algorithm##_init(void *ctx);                    \
  int awslc_prov_##algorithm##_update(void *ctx, const void *data, \
                                      size_t len);                 \
  int awslc_prov_##algorithm##_final(void *ctx, unsigned char *out, \
                                     size_t out_size);             \
  int awslc_prov_##algorithm##_copy(void *dst, const void *src)

// SHA-2, from backend/operations/digests/sha2.c.
AWSLC_PROV_DECLARE_DIGEST_BACKEND(sha224);
AWSLC_PROV_DECLARE_DIGEST_BACKEND(sha256);
AWSLC_PROV_DECLARE_DIGEST_BACKEND(sha384);
AWSLC_PROV_DECLARE_DIGEST_BACKEND(sha512);
AWSLC_PROV_DECLARE_DIGEST_BACKEND(sha512_224);
AWSLC_PROV_DECLARE_DIGEST_BACKEND(sha512_256);

#if defined(__cplusplus)
}  // extern "C"
#endif

#endif  // AWSLC_PROVIDER_INTERNAL_BACKEND_DIGESTS_H
