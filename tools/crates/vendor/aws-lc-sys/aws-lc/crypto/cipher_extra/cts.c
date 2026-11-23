// Copyright (c) 2008-2016 The OpenSSL Project. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <string.h>

#include <openssl/mem.h>
#include <openssl/modes.h>

#include "../internal.h"

// AES Ciphertext Stealing (CTS), CBC-CS3 variant (NIST SP 800-38A Addendum).
//
// CTS extends CBC mode to handle inputs whose length is not a multiple of the
// block size, without padding. This file implements CBC-CS3, in which the last
// two ciphertext blocks are unconditionally swapped -- even for block-aligned
// inputs -- so an exact-block-length message is treated as having a 16-byte
// residue rather than zero residue. This is the convention used by OpenSSL's
// legacy |CRYPTO_cts128_encrypt|/|CRYPTO_cts128_decrypt| API and is the shape
// MIT krb5 1.x calls into for |aes128-cts-hmac-*| and friends (see RFC 3962,
// RFC 8009).
//
// Both entry points return zero if |len| <= 16 (CTS requires at least one
// full block of input plus a partial block; for exact-length inputs callers
// should use plain CBC). On success they return the number of bytes written,
// which equals |len|.

size_t CRYPTO_cts128_encrypt(const uint8_t *in, uint8_t *out, size_t len,
                             const AES_KEY *key, uint8_t ivec[16],
                             cbc128_f cbc) {
  GUARD_PTR(in);
  GUARD_PTR(out);
  GUARD_PTR(key);
  GUARD_PTR(ivec);

  size_t residue = 0;
  uint8_t tmp[16];

  if (len <= 16) {
    return 0;
  }

  assert(!buffers_alias(in, len, out, len));

  if ((residue = len % 16) == 0) {
    residue = 16;
  }
  len -= residue;

  (*cbc)(in, out, len, key, ivec, 1 /* enc */);

  in += len;
  out += len;

  // We don't assume |cbc| handles truncated I/O. Build up a 16-byte buffer
  // containing the |residue| plaintext bytes (zero-padded), CBC-encrypt it,
  // then perform the CTS swap. |tmp| holds plaintext during this step, so it
  // is cleansed before return.
  OPENSSL_memset(tmp, 0, sizeof(tmp));
  OPENSSL_memcpy(tmp, in, residue);
  OPENSSL_memcpy(out, out - 16, residue);
  (*cbc)(tmp, out - 16, 16, key, ivec, 1 /* enc */);

  OPENSSL_cleanse(tmp, sizeof(tmp));
  return len + residue;
}

size_t CRYPTO_cts128_decrypt(const uint8_t *in, uint8_t *out, size_t len,
                             const AES_KEY *key, uint8_t ivec[16],
                             cbc128_f cbc) {
  GUARD_PTR(in);
  GUARD_PTR(out);
  GUARD_PTR(key);
  GUARD_PTR(ivec);

  size_t residue = 0;
  uint8_t tmp[32];

  if (len <= 16) {
    return 0;
  }

  assert(!buffers_alias(in, len, out, len));

  if ((residue = len % 16) == 0) {
    residue = 16;
  }
  len -= 16 + residue;

  if (len) {
    (*cbc)(in, out, len, key, ivec, 0 /* dec */);
    in += len;
    out += len;
  }

  // Decrypt the second-to-last (swapped) ciphertext block in-place into
  // |tmp[0..15]| using a scratch IV at |tmp[16..31]|, so the running |ivec|
  // is preserved for the final XOR step below.
  OPENSSL_memset(tmp, 0, sizeof(tmp));
  (*cbc)(in, tmp, 16, key, tmp + 16, 0 /* dec */);

  // Reconstruct the full last-but-one ciphertext block by overlaying the
  // |residue| stolen ciphertext bytes from the final block. Then decrypt
  // both blocks with the original |ivec|. After the second |cbc| call,
  // |tmp[0..15+residue-1]| holds the trailing plaintext in clear; cleanse
  // before return.
  OPENSSL_memcpy(tmp, in + 16, residue);
  (*cbc)(tmp, tmp, 32, key, ivec, 0 /* dec */);
  OPENSSL_memcpy(out, tmp, 16 + residue);

  OPENSSL_cleanse(tmp, sizeof(tmp));
  return 16 + len + residue;
}
