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

namespace brave_wallet {

static_assert(kSecretboxNonceSize == crypto_secretbox_NONCEBYTES,
              "kSecretboxNonceSize must equal crypto_secretbox_NONCEBYTES");
static_assert(kScryptKeyBytes == crypto_secretbox_KEYBYTES,
              "kScryptKeyBytes must be equal crypto_secretbox_KEYBYTES");

std::optional<std::vector<uint8_t>> ScryptEncrypt(
    base::span<const uint8_t> plaintext,
    base::span<const uint8_t, kScryptKeyBytes> key,
    base::span<const uint8_t, kSecretboxNonceSize> nonce) {
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

std::optional<std::vector<uint8_t>> ScryptDecrypt(
    base::span<const uint8_t> data,
    base::span<const uint8_t, kSecretboxNonceSize> nonce,
    base::span<const uint8_t, kScryptKeyBytes> key) {
  // Reconstruct the full ciphertext with zero bytes prefix
  std::vector<uint8_t> full_ciphertext(
      crypto_secretbox_BOXZEROBYTES + data.size(), 0);
  base::span(full_ciphertext)
      .subspan(base::checked_cast<size_t>(crypto_secretbox_BOXZEROBYTES))
      .copy_from_nonoverlapping(data);

  // Decrypt using NaCl secretbox.
  std::vector<uint8_t> decrypted = full_ciphertext;
  if (crypto_secretbox_open(decrypted.data(), full_ciphertext.data(),
                            full_ciphertext.size(), nonce.data(),
                            key.data()) != 0) {
    return std::nullopt;
  }

  // Extract the payload (skip the zero bytes prefix).
  auto payload = base::span(decrypted).subspan(
      base::checked_cast<size_t>(crypto_secretbox_ZEROBYTES));

  return base::ToVector(payload);
}

std::optional<std::array<uint8_t, crypto_secretbox_KEYBYTES>> ScryptDeriveKey(
    std::string_view password,
    base::span<const uint8_t, kScryptSaltSize> salt,
    const crypto::kdf::ScryptParams& scrypt_params) {
  if (password.empty()) {
    return std::nullopt;
  }

  std::array<uint8_t, crypto_secretbox_KEYBYTES> derived_key;
  if (!crypto::kdf::DeriveKeyScryptNoCheck(
          scrypt_params, base::as_byte_span(password), salt, derived_key)) {
    return std::nullopt;
  }

  return derived_key;
}

}  // namespace brave_wallet
