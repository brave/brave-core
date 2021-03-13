/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SECURITY_CRYPTO_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SECURITY_CRYPTO_UTIL_H_

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace ads {
namespace security {

struct KeyPairInfo;

std::string Sign(const std::map<std::string, std::string>& headers,
                 const std::string& key_id,
                 const std::string& secret_key);

std::vector<uint8_t> Sha256Hash(const std::string& value);

KeyPairInfo GenerateSignKeyPairFromSeed(const std::vector<uint8_t>& seed);

KeyPairInfo GenerateBoxKeyPair();

std::vector<uint8_t> GenerateSecretKeyFromSeed(const std::string& seed_base64);

std::vector<uint8_t> GenerateRandom192BitNonce();

std::vector<uint8_t> Encrypt(const std::vector<uint8_t>& message,
                             const std::vector<uint8_t>& nonce,
                             const std::vector<uint8_t>& public_key,
                             const std::vector<uint8_t>& ephemeral_secret_key);

std::vector<uint8_t> Decrypt(const std::vector<uint8_t>& ciphertext,
                             const std::vector<uint8_t>& nonce,
                             const std::vector<uint8_t>& ephemeral_public_key,
                             const std::vector<uint8_t>& secret_key);

}  // namespace security
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SECURITY_CRYPTO_UTIL_H_
