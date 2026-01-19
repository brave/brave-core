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
#include "crypto/process_bound_string.h"

namespace brave_wallet {

static_assert(kSecretboxNonceSize == crypto_secretbox_NONCEBYTES,
              "kSecretboxNonceSize must equal crypto_secretbox_NONCEBYTES");
static_assert(kScryptKeyBytes == crypto_secretbox_KEYBYTES,
              "kScryptKeyBytes must be equal crypto_secretbox_KEYBYTES");
static_assert(kSecretboxAuthTagSize ==
                  crypto_secretbox_ZEROBYTES - crypto_secretbox_BOXZEROBYTES,
              "kSecretboxAuthTagSize must equal crypto_secretbox_ZEROBYTES - "
              "crypto_secretbox_BOXZEROBYTES");

std::optional<std::vector<uint8_t>> XSalsaPolyEncrypt(
    base::span<const uint8_t> plaintext,
    base::span<const uint8_t, kScryptKeyBytes> key,
    base::span<const uint8_t, kSecretboxNonceSize> nonce) {
  std::vector<uint8_t> padded_plaintext(
      crypto_secretbox_ZEROBYTES + plaintext.size(), 0);

  // Write plaintext with padding.
  base::span(padded_plaintext)
      .last(plaintext.size())
      .copy_from_nonoverlapping(plaintext);

  std::vector<uint8_t> ciphertext(padded_plaintext.size());
  if (crypto_secretbox(ciphertext.data(), padded_plaintext.data(),
                       padded_plaintext.size(), nonce.data(),
                       key.data()) != 0) {
    return std::nullopt;
  }

  // Extract the actual ciphertext (skip the zero bytes prefix).
  auto payload =
      base::span(ciphertext)
          .subspan(base::checked_cast<size_t>(crypto_secretbox_BOXZEROBYTES));

  return base::ToVector(payload);
}

std::optional<SecureVector> XSalsaPolyDecrypt(
    base::span<const uint8_t> data,
    base::span<const uint8_t, kSecretboxNonceSize> nonce,
    base::span<const uint8_t, kScryptKeyBytes> key) {
  // Reconstruct the full ciphertext with zero bytes prefix
  std::vector<uint8_t> full_ciphertext(
      crypto_secretbox_BOXZEROBYTES + data.size(), 0);
  base::span(full_ciphertext).last(data.size()).copy_from_nonoverlapping(data);

  // Decrypt using NaCl secretbox.
  std::vector<uint8_t> decrypted(full_ciphertext.size());
  if (crypto_secretbox_open(decrypted.data(), full_ciphertext.data(),
                            full_ciphertext.size(), nonce.data(),
                            key.data()) != 0) {
    return std::nullopt;
  }

  // Extract the payload (skip the zero bytes prefix).
  auto payload = base::span(decrypted).subspan(
      base::checked_cast<size_t>(crypto_secretbox_ZEROBYTES));
  SecureVector secure_payload(payload.begin(), payload.end());
  crypto::internal::SecureZeroBuffer(decrypted);
  return secure_payload;
}

std::optional<SecureVector> ScryptDeriveKey(
    std::string_view password,
    base::span<const uint8_t> salt,
    const crypto::kdf::ScryptParams& scrypt_params) {
  if (password.empty()) {
    return std::nullopt;
  }

  std::array<uint8_t, crypto_secretbox_KEYBYTES> derived_key = {};
  if (!crypto::kdf::DeriveKeyScryptNoCheck(
          scrypt_params, base::as_byte_span(password), salt, derived_key)) {
    return std::nullopt;
  }

  SecureVector secure_key(derived_key.begin(), derived_key.end());
  crypto::internal::SecureZeroBuffer(derived_key);
  return secure_key;
}

}  // namespace brave_wallet
