/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CRYPTO_KDF_H_
#define BRAVE_CHROMIUM_SRC_CRYPTO_KDF_H_

#include "src/crypto/kdf.h"  // IWYU pragma: export

namespace crypto::kdf {

// Upstream function DeriveKeyScrypt has a CHECK_EQ(rv, 1) which we don't
// want as we shouldn't crash the browser because of bad input data, so this
// is our own version of the same function but returning a bool instead of
// CHECKing.
CRYPTO_EXPORT bool DeriveKeyScryptNoCheck(const ScryptParams& params,
                                          base::span<const uint8_t> password,
                                          base::span<const uint8_t> salt,
                                          base::span<uint8_t> result);

struct Pbkdf2HmacSha256Params {
  // BoringSSL uses a uint32_t for the iteration count for PBKDF2, so we match
  // that.
  uint32_t iterations;
};

CRYPTO_EXPORT bool DeriveKeyPbkdf2HmacSha256(
    const kdf::Pbkdf2HmacSha256Params& params,
    base::span<const uint8_t> password,
    base::span<const uint8_t> salt,
    base::span<uint8_t> result);

}  // namespace crypto::kdf

#endif  // BRAVE_CHROMIUM_SRC_CRYPTO_KDF_H_
