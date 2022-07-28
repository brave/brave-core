/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/conversions/verifiable_conversion_envelope_unittest_util.h"

#include <cstdint>
#include <vector>

#include "base/base64.h"
#include "bat/ads/internal/base/crypto/crypto_unittest_util.h"
#include "bat/ads/internal/base/crypto/crypto_util.h"
#include "bat/ads/internal/conversions/verifiable_conversion_envelope_info.h"
#include "tweetnacl.h"  // NOLINT

namespace ads {
namespace security {

namespace {
constexpr size_t kCryptoBoxZeroBytes = crypto_box_BOXZEROBYTES;
}  // namespace

absl::optional<VerifiableConversionEnvelopeInfo>
GetVerifiableConversionEnvelopeForUserData(const base::Value::Dict& user_data) {
  const base::Value::Dict* value = user_data.FindDict("conversionEnvelope");
  if (!value) {
    return absl::nullopt;
  }

  VerifiableConversionEnvelopeInfo verifiable_conversion_envelope;

  const std::string* algorithm = value->FindString("alg");
  if (algorithm) {
    verifiable_conversion_envelope.algorithm = *algorithm;
  }

  const std::string* ciphertext = value->FindString("ciphertext");
  if (ciphertext) {
    verifiable_conversion_envelope.ciphertext = *ciphertext;
  }

  const std::string* ephemeral_public_key = value->FindString("epk");
  if (ephemeral_public_key) {
    verifiable_conversion_envelope.ephemeral_public_key = *ephemeral_public_key;
  }

  const std::string* nonce = value->FindString("nonce");
  if (nonce) {
    verifiable_conversion_envelope.nonce = *nonce;
  }

  if (!verifiable_conversion_envelope.IsValid()) {
    return absl::nullopt;
  }

  return verifiable_conversion_envelope;
}

absl::optional<std::string> OpenEnvelope(
    const VerifiableConversionEnvelopeInfo verifiable_conversion_envelope,
    const std::string& advertiser_secret_key_base64) {
  DCHECK(!advertiser_secret_key_base64.empty());

  if (!verifiable_conversion_envelope.IsValid()) {
    return absl::nullopt;
  }

  const absl::optional<std::vector<uint8_t>> ciphertext_optional =
      base::Base64Decode(verifiable_conversion_envelope.ciphertext);
  if (!ciphertext_optional) {
    return absl::nullopt;
  }
  std::vector<uint8_t> ciphertext = ciphertext_optional.value();

  // API requires 16 leading zero-padding bytes
  ciphertext.insert(ciphertext.cbegin(), kCryptoBoxZeroBytes, 0);

  const absl::optional<std::vector<uint8_t>> nonce_optional =
      base::Base64Decode(verifiable_conversion_envelope.nonce);
  if (!nonce_optional) {
    return absl::nullopt;
  }
  const std::vector<uint8_t>& nonce = nonce_optional.value();

  const absl::optional<std::vector<uint8_t>> ephemeral_public_key_optional =
      base::Base64Decode(verifiable_conversion_envelope.ephemeral_public_key);
  if (!ephemeral_public_key_optional) {
    return absl::nullopt;
  }
  std::vector<uint8_t> ephemeral_public_key =
      ephemeral_public_key_optional.value();

  const absl::optional<std::vector<uint8_t>> advertiser_secret_key_optional =
      base::Base64Decode(advertiser_secret_key_base64);
  if (!advertiser_secret_key_optional) {
    return absl::nullopt;
  }
  const std::vector<uint8_t>& advertiser_secret_key =
      advertiser_secret_key_optional.value();

  const std::vector<uint8_t> plaintext =
      Decrypt(ciphertext, nonce, ephemeral_public_key, advertiser_secret_key);

  return std::string((const char*)&plaintext.front());
}

absl::optional<std::string> OpenEvenlopeForUserDataAndAdvertiserSecretKey(
    const base::Value::Dict& user_data,
    const std::string& advertiser_secret_key) {
  const absl::optional<VerifiableConversionEnvelopeInfo>&
      verifiable_conversion_envelope_optional =
          GetVerifiableConversionEnvelopeForUserData(user_data);
  if (!verifiable_conversion_envelope_optional) {
    return absl::nullopt;
  }
  const VerifiableConversionEnvelopeInfo& verifiable_conversion_envelope =
      verifiable_conversion_envelope_optional.value();

  const absl::optional<std::string>& message_optional =
      OpenEnvelope(verifiable_conversion_envelope, advertiser_secret_key);
  if (!message_optional) {
    return absl::nullopt;
  }
  const std::string message = message_optional.value();

  return message;
}

}  // namespace security
}  // namespace ads
