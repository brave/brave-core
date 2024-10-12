/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CRYPTO_KDF_H_
#define BRAVE_CHROMIUM_SRC_CRYPTO_KDF_H_

#include "src/crypto/kdf.h"  // IWYU pragma: export

namespace crypto::kdf {

CRYPTO_EXPORT void DeriveKeyPbkdf2HmacSha256(
    const kdf::Pbkdf2HmacSha1Params& params,
    base::span<const uint8_t> password,
    base::span<const uint8_t> salt,
    base::span<uint8_t> result,
    crypto::SubtlePassKey);

}  // namespace crypto::kdf

#endif  // BRAVE_CHROMIUM_SRC_CRYPTO_KDF_H_
