/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/crypto/kdf.cc"

namespace crypto::kdf {

void DeriveKeyPbkdf2HmacSha256(const kdf::Pbkdf2HmacSha1Params& params,
                               base::span<const uint8_t> password,
                               base::span<const uint8_t> salt,
                               base::span<uint8_t> result,
                               crypto::SubtlePassKey) {
  OpenSSLErrStackTracer err_tracer(FROM_HERE);
  int rv = PKCS5_PBKDF2_HMAC(base::as_chars(password).data(), password.size(),
                             salt.data(), salt.size(), params.iterations,
                             EVP_sha256(), result.size(), result.data());

  CHECK_EQ(rv, 1);
}

}  // namespace crypto::kdf
