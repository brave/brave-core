/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SCRYPT_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SCRYPT_UTILS_H_

#include <array>
#include <optional>
#include <string_view>
#include <vector>

#include "base/containers/span.h"
#include "crypto/kdf.h"

namespace brave_wallet {

inline constexpr uint8_t kScryptSaltSize = 32u;
// NaCl secretbox nonce size (24 bytes) equal to tweetnacl
// crypto_secretbox_NONCEBYTES.
inline constexpr uint8_t kSecretboxNonceSize = 24u;

// Result structure containing encrypted data, nonce, and salt.
struct ScryptEncryptResult {
  ScryptEncryptResult();
  ~ScryptEncryptResult();
  ScryptEncryptResult(const ScryptEncryptResult&);
  ScryptEncryptResult& operator=(const ScryptEncryptResult&);
  ScryptEncryptResult(ScryptEncryptResult&&);
  ScryptEncryptResult& operator=(ScryptEncryptResult&&);

  // Encrypted ciphertext without zero bytes prefix (skips BOXZEROBYTES).
  std::vector<uint8_t> data;
  // Nonce used for xsalsa20-poly1305 encryption.
  std::array<uint8_t, kSecretboxNonceSize> nonce;
  // Salt used for scrypt key derivation.
  std::array<uint8_t, kScryptSaltSize> salt;
};

// Encrypts data using scrypt key derivation and xsalsa20-poly1305 encryption.
// Returns a structure containing the ciphertext, nonce, and salt.
// If salt or nonce are provided (for testing), they will be used instead of
// generating random ones.
std::optional<ScryptEncryptResult> ScryptEncrypt(
    base::span<const uint8_t> plaintext,
    std::string_view password,
    const crypto::kdf::ScryptParams& scrypt_params,
    std::optional<base::span<const uint8_t, kScryptSaltSize>> salt =
        std::nullopt,
    std::optional<base::span<const uint8_t, kSecretboxNonceSize>> nonce =
        std::nullopt);

// Decrypts data encrypted with ScryptEncrypt.
// Returns the decrypted plaintext, or std::nullopt if decryption fails
// (e.g., wrong password, wrong scrypt params, corrupted data).
std::optional<std::vector<uint8_t>> ScryptDecrypt(
    const ScryptEncryptResult& encrypted,
    std::string_view password,
    const crypto::kdf::ScryptParams& scrypt_params);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SCRYPT_UTILS_H_
