/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <third_party/boringssl/src/include/openssl/curve25519.h>  // IWYU pragma: export

#if defined(__cplusplus)
extern "C" {
#endif

// https://datatracker.ietf.org/doc/html/rfc8032#section-5.1.5
// requires scalar to follow this requirements 'The lowest 3 bits of the first
// octet are cleared, the highest bit of the last octet is cleared, and the
// second highest bit of the last octet is set'.
OPENSSL_EXPORT int ED25519_is_scalar_pruned(const uint8_t scalar[32]);

// Produces pubkey form scalar.
// Function fails if `scalar` is not pruned.
// https://www.rfc-editor.org/rfc/rfc8032.html#section-5.1.5 See
// `ED25519_keypair_from_seed` as origin.
OPENSSL_EXPORT int ED25519_pubkey_from_scalar(uint8_t out_public_key[32],
                                              const uint8_t scalar[32]);

// Same as `ED25519_sign` but without hashing private key. `scalar` and `prefix`
// come from ED25519_BIP32 algorithm.
// Function fails if `scalar` is not pruned.
// https://www.rfc-editor.org/rfc/rfc8032.html#section-5.1.5
OPENSSL_EXPORT int ED25519_sign_with_scalar_and_prefix(
    uint8_t out_sig[64],
    const uint8_t* message,
    size_t message_len,
    const uint8_t scalar[32],
    const uint8_t prefix[32],
    const uint8_t public_key[32]);

#if defined(__cplusplus)
}  // extern C
#endif
