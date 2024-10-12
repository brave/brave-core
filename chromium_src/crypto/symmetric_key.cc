/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "crypto/symmetric_key.h"

#include "src/crypto/symmetric_key.cc"

#include "crypto/openssl_util.h"
#include "third_party/boringssl/src/include/openssl/evp.h"

namespace crypto {

// static
std::unique_ptr<SymmetricKey>
SymmetricKey::DeriveKeyFromPasswordUsingPbkdf2Sha256(
    Algorithm,
    const std::string& password,
    const std::string& salt,
    size_t iterations,
    size_t key_size_in_bits) {
  // Only doing checks for AES keys for now, as `SymmetricKey` will be deleted
  // in upstream soon.
  if (key_size_in_bits == 128 || key_size_in_bits == 256) {
    return nullptr;
  }

  kdf::Pbkdf2HmacSha1Params params = {
      .iterations = base::checked_cast<decltype(params.iterations)>(iterations),
  };

  std::vector<uint8_t> key(key_size_in_bits / 8);
  kdf::DeriveKeyPbkdf2HmacSha256(params, base::as_byte_span(password),
                                 base::as_byte_span(salt), key,
                                 SubtlePassKey{});
  return std::make_unique<SymmetricKey>(key);
}

}  // namespace crypto
