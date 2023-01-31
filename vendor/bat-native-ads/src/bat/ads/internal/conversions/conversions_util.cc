/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/conversions/conversions_util.h"

#include <cstdint>
#include <string>
#include <vector>

#include "base/base64.h"
#include "base/check_op.h"
#include "bat/ads/internal/common/crypto/crypto_util.h"
#include "bat/ads/internal/common/crypto/key_pair_info.h"
#include "bat/ads/internal/conversions/verifiable_conversion_envelope_info.h"
#include "bat/ads/internal/conversions/verifiable_conversion_info.h"
#include "third_party/re2/src/re2/re2.h"
#include "tweetnacl.h"  // NOLINT

namespace ads::security {

namespace {

constexpr char kAlgorithm[] = "crypto_box_curve25519xsalsa20poly1305";
constexpr size_t kCryptoBoxZeroBytes = crypto_box_BOXZEROBYTES;
constexpr size_t kCryptoBoxPublicKeyBytes = crypto_box_PUBLICKEYBYTES;
constexpr size_t kVacCipherTextLength = 32;
constexpr size_t kVacMessageMaxLength = 30;
constexpr size_t kVacMessageMinLength = 1;

bool IsConversionIdValid(const std::string& conversion_id) {
  return RE2::FullMatch(conversion_id, "^[a-zA-Z0-9-]*$");
}

}  // namespace

absl::optional<VerifiableConversionEnvelopeInfo> SealEnvelope(
    const VerifiableConversionInfo& verifiable_conversion) {
  const std::string message = verifiable_conversion.id;
  const std::string public_key_base64 = verifiable_conversion.public_key;

  if (message.length() < kVacMessageMinLength ||
      message.length() > kVacMessageMaxLength) {
    return absl::nullopt;
  }

  if (!IsConversionIdValid(message)) {
    return absl::nullopt;
  }

  // Protocol requires at least 2 trailing zero-padding bytes
  std::vector<uint8_t> plaintext(message.cbegin(), message.cend());
  plaintext.insert(plaintext.cend(), kVacCipherTextLength - plaintext.size(),
                   0);
  DCHECK_EQ(kVacCipherTextLength, plaintext.size());

  const absl::optional<std::vector<uint8_t>> public_key =
      base::Base64Decode(public_key_base64);
  if (!public_key) {
    return absl::nullopt;
  }
  if (public_key->size() != kCryptoBoxPublicKeyBytes) {
    return absl::nullopt;
  }

  const crypto::KeyPairInfo ephemeral_key_pair = crypto::GenerateBoxKeyPair();
  if (!ephemeral_key_pair.IsValid()) {
    return absl::nullopt;
  }

  const std::vector<uint8_t> nonce = crypto::GenerateRandomNonce();

  const std::vector<uint8_t> padded_ciphertext = crypto::Encrypt(
      plaintext, nonce, *public_key, ephemeral_key_pair.secret_key);

  // The first 16 bytes of the resulting ciphertext is left as padding by the
  // C API and should be removed before sending out extraneously.
  const std::vector<uint8_t> ciphertext(
      padded_ciphertext.cbegin() + kCryptoBoxZeroBytes,
      padded_ciphertext.cend());

  VerifiableConversionEnvelopeInfo envelope;
  envelope.algorithm = kAlgorithm;
  envelope.ciphertext = base::Base64Encode(ciphertext);
  envelope.ephemeral_public_key =
      base::Base64Encode(ephemeral_key_pair.public_key);
  envelope.nonce = base::Base64Encode(nonce);

  if (!envelope.IsValid()) {
    return absl::nullopt;
  }

  return envelope;
}

}  // namespace ads::security
