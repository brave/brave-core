/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_CRYPTO_CRYPTO_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_CRYPTO_CRYPTO_UTIL_H_

#include <cstdint>
#include <string>
#include <vector>

#include "absl/types/optional.h"

namespace ads::crypto {

struct KeyPairInfo;

std::vector<uint8_t> Sha256(const std::string& value);

absl::optional<KeyPairInfo> GenerateSignKeyPairFromSeed(
    const std::vector<uint8_t>& seed);
KeyPairInfo GenerateBoxKeyPair();

std::vector<uint8_t> GenerateRandomNonce();

absl::optional<std::string> Sign(const std::string& message,
                                 const std::string& secret_key);
bool Verify(const std::string& message,
            const std::string& public_key,
            const std::string& signature);

std::vector<uint8_t> Encrypt(const std::vector<uint8_t>& plaintext,
                             const std::vector<uint8_t>& nonce,
                             const std::vector<uint8_t>& public_key,
                             const std::vector<uint8_t>& secret_key);
std::vector<uint8_t> Decrypt(const std::vector<uint8_t>& ciphertext,
                             const std::vector<uint8_t>& nonce,
                             const std::vector<uint8_t>& public_key,
                             const std::vector<uint8_t>& secret_key);

}  // namespace ads::crypto

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_CRYPTO_CRYPTO_UTIL_H_
