/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/envelope/verifiable_conversion_envelope_unittest_util.h"

#include <cstdint>
#include <vector>

#include "base/base64.h"
#include "brave/components/brave_ads/core/internal/account/user_data/fixed/conversion_user_data_constants.h"
#include "brave/components/brave_ads/core/internal/common/crypto/crypto_util.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/envelope/verifiable_conversion_envelope_info.h"
#include "tweetnacl.h"  // NOLINT

namespace brave_ads {

absl::optional<VerifiableConversionEnvelopeInfo>
MaybeBuildVerifiableConversionEnvelopeForTesting(
    const base::Value::Dict& user_data) {
  const auto* const dict = user_data.FindDict(kVerifiableConversionEnvelopeKey);
  if (!dict) {
    return absl::nullopt;
  }

  VerifiableConversionEnvelopeInfo verifiable_conversion_envelope;

  const std::string* const algorithm =
      dict->FindString(kVerifiableConversionEnvelopeAlgorithmKey);
  if (algorithm) {
    verifiable_conversion_envelope.algorithm = *algorithm;
  }

  const std::string* const ciphertext =
      dict->FindString(kVerifiableConversionEnvelopeCipherTextKey);
  if (ciphertext) {
    verifiable_conversion_envelope.ciphertext = *ciphertext;
  }

  const std::string* const ephemeral_public_key =
      dict->FindString(kVerifiableConversionEnvelopeEphemeralPublicKeyKey);
  if (ephemeral_public_key) {
    verifiable_conversion_envelope.ephemeral_public_key = *ephemeral_public_key;
  }

  const std::string* const nonce =
      dict->FindString(kVerifiableConversionEnvelopeNonceKey);
  if (nonce) {
    verifiable_conversion_envelope.nonce = *nonce;
  }

  if (!verifiable_conversion_envelope.IsValid()) {
    return absl::nullopt;
  }

  return verifiable_conversion_envelope;
}

absl::optional<std::string> OpenVerifiableConversionEnvelopeForTesting(
    const VerifiableConversionEnvelopeInfo& verifiable_conversion_envelope,
    const std::string& advertiser_secret_key_base64) {
  CHECK(!advertiser_secret_key_base64.empty());

  if (!verifiable_conversion_envelope.IsValid()) {
    return absl::nullopt;
  }

  absl::optional<std::vector<uint8_t>> ciphertext =
      base::Base64Decode(verifiable_conversion_envelope.ciphertext);
  if (!ciphertext) {
    return absl::nullopt;
  }

  // API requires 16 leading zero-padding bytes
  ciphertext->insert(ciphertext->cbegin(), crypto_box_BOXZEROBYTES, 0);

  const absl::optional<std::vector<uint8_t>> nonce =
      base::Base64Decode(verifiable_conversion_envelope.nonce);
  if (!nonce) {
    return absl::nullopt;
  }

  const absl::optional<std::vector<uint8_t>> ephemeral_public_key =
      base::Base64Decode(verifiable_conversion_envelope.ephemeral_public_key);
  if (!ephemeral_public_key) {
    return absl::nullopt;
  }

  const absl::optional<std::vector<uint8_t>> advertiser_secret_key =
      base::Base64Decode(advertiser_secret_key_base64);
  if (!advertiser_secret_key) {
    return absl::nullopt;
  }

  const std::vector<uint8_t> plaintext = crypto::Decrypt(
      *ciphertext, *nonce, *ephemeral_public_key, *advertiser_secret_key);

  return std::string(reinterpret_cast<const char*>(plaintext.data()));
}

}  // namespace brave_ads
