/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/boringssl/src/include/openssl/curve25519.h"

// Unfortunately we have to suppress `-Wheader-hygiene` because the compiler
// treats `curve25519.cc` as a header file when included from this shadow
// source, and this causes this warning to go off with:
//
// error: using namespace directive in global context in header.
//    44 | using namespace bssl;
//       |
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wheader-hygiene"

#include <third_party/boringssl/src/crypto/curve25519/curve25519.cc>

#pragma clang diagnostic pop

#ifdef UNSAFE_BUFFERS_BUILD
#pragma allow_unsafe_buffers
#endif

int ED25519_is_scalar_pruned(const uint8_t scalar[32]) {
  return (scalar[0] & 0b00000111) == 0b00000000 &&
         (scalar[31] & 0b11000000) == 0b01000000;
}

int ED25519_pubkey_from_scalar(uint8_t out_public_key[32],
                               const uint8_t scalar[32]) {
  if (!ED25519_is_scalar_pruned(scalar)) {
    return 0;
  }

  ge_p3 A;

  x25519_ge_scalarmult_base(&A, scalar);
  ge_p3_tobytes(out_public_key, &A);

  CONSTTIME_DECLASSIFY(out_public_key, 32);

  return 1;
}

int ED25519_sign_with_scalar_and_prefix(uint8_t out_sig[64],
                                        const uint8_t* message,
                                        size_t message_len,
                                        const uint8_t scalar[32],
                                        const uint8_t prefix[32],
                                        const uint8_t public_key[32]) {
  if (!ED25519_is_scalar_pruned(scalar)) {
    return 0;
  }

  SHA512_CTX hash_ctx;
  SHA512_Init(&hash_ctx);
  SHA512_Update(&hash_ctx, prefix, 32);
  SHA512_Update(&hash_ctx, message, message_len);
  uint8_t nonce[SHA512_DIGEST_LENGTH];
  SHA512_Final(nonce, &hash_ctx);

  x25519_sc_reduce(nonce);
  ge_p3 R;
  x25519_ge_scalarmult_base(&R, nonce);
  ge_p3_tobytes(out_sig, &R);

  SHA512_Init(&hash_ctx);
  SHA512_Update(&hash_ctx, out_sig, 32);
  SHA512_Update(&hash_ctx, public_key, 32);
  SHA512_Update(&hash_ctx, message, message_len);
  uint8_t hram[SHA512_DIGEST_LENGTH];
  SHA512_Final(hram, &hash_ctx);

  x25519_sc_reduce(hram);
  sc_muladd(out_sig + 32, hram, scalar, nonce);

  // The signature is computed from the private key, but is public.
  CONSTTIME_DECLASSIFY(out_sig, 64);
  return 1;
}
