// Copyright (c) 2008-2016 The OpenSSL Project. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef OPENSSL_HEADER_MODES_H
#define OPENSSL_HEADER_MODES_H

#include <openssl/aes.h>
#include <openssl/base.h>

#if defined(__cplusplus)
extern "C" {
#endif


// Block-cipher mode helpers compatible with the OpenSSL <openssl/modes.h>
// API surface. AWS-LC currently exposes only the helpers needed by external
// consumers (in particular MIT krb5's CBC-CTS code path); the |cbc128_f|
// typedef and Ciphertext Stealing routines below are the supported set.
//
// AES is the only block cipher these helpers are tested against. The
// |AES_KEY|-typed functions are intentionally narrow: callers supply an
// |AES_KEY| set up via |AES_set_encrypt_key| (or |AES_set_decrypt_key| for
// decryption) and a CBC primitive function (|AES_cbc_encrypt|).

// cbc128_f is the type of a function that performs CBC-mode encryption
// (|enc| = 1) or decryption (|enc| = 0) using |key| and the supplied IV.
// This matches the signature of |AES_cbc_encrypt|.
typedef void (*cbc128_f)(const uint8_t *in, uint8_t *out, size_t len,
                         const AES_KEY *key, uint8_t ivec[16], int enc);

// CRYPTO_cts128_encrypt encrypts |len| bytes from |in| to |out| in CBC
// Ciphertext Stealing (CTS) mode (CS3 / NIST SP 800-38A Addendum convention:
// the last two ciphertext blocks are unconditionally swapped, and an
// exact-block-multiple input is handled as a 16-byte residue). The IV is
// updated in |ivec|. |key| must have been set up for encryption.
//
// Returns the number of bytes written, equal to |len|, on success. Returns
// zero if |len| <= 16; for inputs of one block or fewer use plain CBC.
//
// |in| and |out| may not alias.
OPENSSL_EXPORT size_t CRYPTO_cts128_encrypt(const uint8_t *in, uint8_t *out,
                                            size_t len, const AES_KEY *key,
                                            uint8_t ivec[16], cbc128_f cbc);

// CRYPTO_cts128_decrypt decrypts a CTS-mode ciphertext produced by
// |CRYPTO_cts128_encrypt| (or a compatible producer). The IV is updated in
// |ivec|. |key| must have been set up for decryption. Returns the number of
// bytes written, equal to |len|, on success; zero if |len| <= 16.
//
// |in| and |out| may not alias.
OPENSSL_EXPORT size_t CRYPTO_cts128_decrypt(const uint8_t *in, uint8_t *out,
                                            size_t len, const AES_KEY *key,
                                            uint8_t ivec[16], cbc128_f cbc);


#if defined(__cplusplus)
}  // extern C
#endif

#endif  // OPENSSL_HEADER_MODES_H
