/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/scrypt_utils.h"

#include "base/check.h"
#include "base/containers/span.h"
#include "base/containers/span_writer.h"
#include "base/containers/to_vector.h"
#include "base/numerics/safe_conversions.h"
#include "brave/vendor/bat-native-tweetnacl/tweetnacl.h"
#include "crypto/kdf.h"
#include "crypto/random.h"

namespace brave_wallet {

static_assert(kSecretboxNonceSize == crypto_secretbox_NONCEBYTES,
              "kSecretboxNonceSize must equal crypto_secretbox_NONCEBYTES");

ScryptEncryptResult::ScryptEncryptResult() = default;
ScryptEncryptResult::~ScryptEncryptResult() = default;
ScryptEncryptResult::ScryptEncryptResult(const ScryptEncryptResult&) = default;
ScryptEncryptResult& ScryptEncryptResult::operator=(
    const ScryptEncryptResult&) = default;
ScryptEncryptResult::ScryptEncryptResult(ScryptEncryptResult&&) = default;
ScryptEncryptResult& ScryptEncryptResult::operator=(ScryptEncryptResult&&) =
    default;

std::optional<ScryptEncryptResult> ScryptEncrypt(
    base::span<const uint8_t> plaintext,
    std::string_view password,
    const crypto::kdf::ScryptParams& scrypt_params,
    std::optional<base::span<const uint8_t, kScryptSaltSize>> salt,
    std::optional<base::span<const uint8_t, kSecretboxNonceSize>> nonce) {
  if (password.empty()) {
    return std::nullopt;
  }

  ScryptEncryptResult result;

  // Generate or use provided salt for scrypt.
  if (salt.has_value()) {
    base::span(result.salt).copy_from_nonoverlapping(*salt);
  } else {
    crypto::RandBytes(result.salt);
  }

  // Derive encryption key from password using scrypt
  std::array<uint8_t, crypto_secretbox_KEYBYTES> derived_key;
  if (!crypto::kdf::DeriveKeyScryptNoCheck(scrypt_params,
                                           base::as_byte_span(password),
                                           result.salt, derived_key)) {
    return std::nullopt;
  }

  // Generate or use provided nonce for xsalsa20-poly1305.
  if (nonce.has_value()) {
    base::span(result.nonce).copy_from_nonoverlapping(*nonce);
  } else {
    crypto::RandBytes(result.nonce);
  }

  // Encrypt the plaintext using NaCl secretbox (xsalsa20-poly1305).
  std::vector<uint8_t> padded_plaintext(
      plaintext.size() + crypto_secretbox_ZEROBYTES, 0);
  std::vector<uint8_t> ciphertext = padded_plaintext;

  // Write plaintext with padding.
  base::SpanWriter padded_plaintext_writer = base::SpanWriter(
      base::span(padded_plaintext)
          .subspan(base::checked_cast<size_t>(crypto_secretbox_ZEROBYTES)));
  padded_plaintext_writer.Write(plaintext);
  CHECK_EQ(padded_plaintext_writer.remaining(), 0u);

  if (crypto_secretbox(ciphertext.data(), padded_plaintext.data(),
                       padded_plaintext.size(), result.nonce.data(),
                       derived_key.data()) != 0) {
    return std::nullopt;
  }

  // Extract the actual ciphertext (skip the zero bytes prefix).
  auto payload =
      base::span(ciphertext)
          .subspan(base::checked_cast<size_t>(crypto_secretbox_BOXZEROBYTES));

  result.data = base::ToVector(payload);
  return result;
}

std::optional<std::vector<uint8_t>> ScryptDecrypt(
    const ScryptEncryptResult& encrypted,
    std::string_view password,
    const crypto::kdf::ScryptParams& scrypt_params) {
  if (password.empty()) {
    return std::nullopt;
  }

  // Derive the same encryption key from password using scrypt.
  std::array<uint8_t, crypto_secretbox_KEYBYTES> derived_key;
  if (!crypto::kdf::DeriveKeyScryptNoCheck(scrypt_params,
                                           base::as_byte_span(password),
                                           encrypted.salt, derived_key)) {
    return std::nullopt;
  }

  // Reconstruct the full ciphertext with zero bytes prefix
  std::vector<uint8_t> full_ciphertext(
      crypto_secretbox_BOXZEROBYTES + encrypted.data.size(), 0);
  base::span(full_ciphertext)
      .subspan(base::checked_cast<size_t>(crypto_secretbox_BOXZEROBYTES))
      .copy_from_nonoverlapping(encrypted.data);

  // Decrypt using NaCl secretbox.
  std::vector<uint8_t> decrypted = full_ciphertext;
  if (crypto_secretbox_open(decrypted.data(), full_ciphertext.data(),
                            full_ciphertext.size(), encrypted.nonce.data(),
                            derived_key.data()) != 0) {
    return std::nullopt;
  }

  // Extract the plaintext (skip the zero bytes prefix).
  auto plaintext = base::span(decrypted).subspan(
      base::checked_cast<size_t>(crypto_secretbox_ZEROBYTES));

  return base::ToVector(plaintext);
}

}  // namespace brave_wallet
