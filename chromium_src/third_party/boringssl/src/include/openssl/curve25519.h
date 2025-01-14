/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/third_party/boringssl/src/include/openssl/curve25519.h"  // IWYU pragma: export

#if defined(__cplusplus)
extern "C" {
#endif

OPENSSL_EXPORT int ED25519_pubkey_from_scalar(uint8_t out_public_key[32],
                                              const uint8_t scalar[32]);
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
