// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

#include "internal.h"

#include "../internal.h"

// Keccak-256 (Ethereum-style, original 0x01 padding). NOT FIPS-approved.
//
// This reuses the FIPS module's FIPS 202 buffering primitives and Keccak-f[1600]
// permutation but initialises the context with the original Keccak padding byte
// (|KECCAK256_PAD_CHAR|) instead of the FIPS 202 ones. Because the module's
// |KeccakSponge_Init| deliberately rejects non-FIPS-202 padding, we set up the
// context here rather than calling it.

int Keccak256_Init(KECCAK1600_CTX *ctx) {
  if (ctx == NULL) {
    return 0;
  }

  // |KeccakSponge_Init| bounds its |block_size| argument against the context
  // buffer at runtime because there it is variable. Keccak-256's block size is
  // a compile-time constant, so assert it statically instead.
  OPENSSL_STATIC_ASSERT(KECCAK256_CBLOCK <= sizeof(ctx->buf),
                        keccak256_block_size_exceeds_ctx_buffer)

  KeccakSponge_Reset(ctx);
  ctx->block_size = KECCAK256_CBLOCK;
  ctx->md_size = KECCAK256_DIGEST_LENGTH;
  ctx->pad = KECCAK256_PAD_CHAR;
  return 1;
}

int Keccak256_Update(KECCAK1600_CTX *ctx, const void *data, size_t len) {
  if (ctx == NULL) {
    return 0;
  }
  if (data == NULL && len != 0) {
    return 0;
  }
  if (len == 0) {
    return 1;
  }
  // As in |Keccak256_Final|, refuse a zeroed context rather than letting it
  // reach |KeccakSponge_Absorb|, where |Keccak1600_Absorb| would spin forever
  // on |while (len >= r)| with |r == 0|. Absorbing into a context that was
  // never initialised, or that EVP has already finalised and cleansed, is a
  // caller error, so this reports failure rather than silently doing nothing.
  if (ctx->block_size == 0) {
    return 0;
  }
  return KeccakSponge_Absorb(ctx, data, len);
}

int Keccak256_Final(uint8_t out[KECCAK256_DIGEST_LENGTH], KECCAK1600_CTX *ctx) {
  if (out == NULL || ctx == NULL) {
    return 0;
  }
  // A zeroed context reaches here whenever |Keccak256_Init| was skipped, and
  // also on a second |Keccak256_Final| through EVP: |EVP_DigestFinal_ex|
  // cleanses |md_data| on the way out. Bail out first, because the callees below
  // assume an initialised context: |KeccakSponge_AbsorbFinal| assumes
  // |block_size| is non-zero and would index |ctx->buf[block_size - 1]| out of
  // bounds, and |Keccak1600_Absorb| assumes the same and would loop forever on
  // |r == 0|. |SHA3_Final| guards the same way.
  if (ctx->md_size == 0) {
    return 1;
  }
  if (KeccakSponge_AbsorbFinal(out, ctx) == 0) {
    return 0;
  }
  Keccak1600_Squeeze(ctx->A, out, ctx->md_size, ctx->block_size, ctx->state);
  ctx->state = KECCAK1600_STATE_FINAL;
  // Intentionally no FIPS_service_indicator_update_state(): Keccak-256 with
  // 0x01 padding is not an approved service.
  return 1;
}

uint8_t *Keccak256(const uint8_t *data, size_t len,
                   uint8_t out[KECCAK256_DIGEST_LENGTH]) {
  KECCAK1600_CTX ctx;
  int ok = (Keccak256_Init(&ctx) &&
            Keccak256_Update(&ctx, data, len) &&
            Keccak256_Final(out, &ctx));

  OPENSSL_cleanse(&ctx, sizeof(ctx));
  if (ok == 0) {
    return NULL;
  }
  return out;
}
