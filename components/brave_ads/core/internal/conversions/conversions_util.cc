/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/conversions_util.h"

#include <cstdint>
#include <vector>

#include "base/base64.h"
#include "base/check_op.h"
#include "brave/components/brave_ads/core/internal/common/crypto/crypto_util.h"
#include "brave/components/brave_ads/core/internal/common/crypto/key_pair_info.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions_util_constants.h"
#include "brave/components/brave_ads/core/internal/conversions/verifiable_conversion_envelope_info.h"
#include "brave/components/brave_ads/core/internal/conversions/verifiable_conversion_info.h"
#include "third_party/re2/src/re2/re2.h"
#include "tweetnacl.h"  // NOLINT

namespace brave_ads {

namespace {

constexpr char kAlgorithm[] = "crypto_box_curve25519xsalsa20poly1305";
constexpr size_t kCipherTextLength = 32;

bool IsConversionIdValid(const std::string& conversion_id) {
  return RE2::FullMatch(conversion_id, "^[a-zA-Z0-9-]*$");
}

}  // namespace

std::string GetAlgorithm() {
  return kAlgorithm;
}

absl::optional<VerifiableConversionEnvelopeInfo> SealEnvelope(
    const VerifiableConversionInfo& verifiable_conversion) {
  const std::string message = verifiable_conversion.id;
  const std::string public_key_base64 = verifiable_conversion.public_key;

  if (message.length() < kMinVerifiableConversionMessageLength ||
      message.length() > kMaxVerifiableConversionMessageLength) {
    return absl::nullopt;
  }

  if (!IsConversionIdValid(message)) {
    return absl::nullopt;
  }

  // Protocol requires at least 2 trailing zero-padding bytes
  std::vector<uint8_t> plaintext(message.cbegin(), message.cend());
  plaintext.insert(plaintext.cend(), kCipherTextLength - plaintext.size(), 0);
  CHECK_EQ(kCipherTextLength, plaintext.size());

  const absl::optional<std::vector<uint8_t>> public_key =
      base::Base64Decode(public_key_base64);
  if (!public_key) {
    return absl::nullopt;
  }
  if (public_key->size() != crypto_box_PUBLICKEYBYTES) {
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
      padded_ciphertext.cbegin() + crypto_box_BOXZEROBYTES,
      padded_ciphertext.cend());

  VerifiableConversionEnvelopeInfo envelope;
  envelope.algorithm = GetAlgorithm();
  envelope.ciphertext = base::Base64Encode(ciphertext);
  envelope.ephemeral_public_key =
      base::Base64Encode(ephemeral_key_pair.public_key);
  envelope.nonce = base::Base64Encode(nonce);

  if (!envelope.IsValid()) {
    return absl::nullopt;
  }

  return envelope;
}

}  // namespace brave_ads
