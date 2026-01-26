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
#include "brave/components/brave_wallet/browser/internal/hd_key.h"
#include "crypto/kdf.h"
#include "crypto/process_bound_string.h"

namespace brave_wallet {

inline constexpr uint8_t kScryptSaltSize = 32u;
// NaCl secretbox nonce size (24 bytes) equal to tweetnacl
// crypto_secretbox_NONCEBYTES.
inline constexpr uint8_t kSecretboxNonceSize = 24u;
// NaCl secretbox key size (24 bytes) equal to tweetnacl
// crypto_secretbox_KEYBYTES.
inline constexpr uint8_t kScryptKeyBytes = 32u;
// Size of encoded data prefix.
inline constexpr uint8_t kSecretboxAuthTagSize = 16u;

// Encrypts data using xsalsa20-poly1305 encryption with the provided key.
std::optional<std::vector<uint8_t>> XSalsaPolyEncrypt(
    base::span<const uint8_t> plaintext,
    base::span<const uint8_t, kScryptKeyBytes> key,
    base::span<const uint8_t, kSecretboxNonceSize> nonce);

// Decrypts data encrypted with ScryptEncrypt.
// Returns the decrypted plaintext, or std::nullopt if decryption fails
// (e.g., wrong key, corrupted data).
std::optional<SecureVector> XSalsaPolyDecrypt(
    base::span<const uint8_t> data,
    base::span<const uint8_t, kSecretboxNonceSize> nonce,
    base::span<const uint8_t, kScryptKeyBytes> key);

// Derives an encryption key from a password using scrypt key derivation.
// Returns the derived key size of kScryptKeyBytes,
// or std::nullopt if key derivation fails.
std::optional<SecureVector> ScryptDeriveKey(
    std::string_view password,
    base::span<const uint8_t> salt,
    const crypto::kdf::ScryptParams& scrypt_params);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SCRYPT_UTILS_H_
