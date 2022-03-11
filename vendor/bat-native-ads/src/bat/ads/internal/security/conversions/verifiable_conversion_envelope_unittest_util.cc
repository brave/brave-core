/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/security/conversions/verifiable_conversion_envelope_unittest_util.h"

#include <cstdint>
#include <vector>

#include "base/values.h"
#include "bat/ads/internal/base64_util.h"
#include "bat/ads/internal/security/conversions/verifiable_conversion_envelope_info.h"
#include "bat/ads/internal/security/crypto_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "tweetnacl.h"  // NOLINT

namespace ads {
namespace security {

namespace {
const size_t kCryptoBoxZeroBytes = crypto_box_BOXZEROBYTES;
}  // namespace

absl::optional<VerifiableConversionEnvelopeInfo>
GetVerifiableConversionEnvelopeForUserData(const base::Value& user_data) {
  const base::Value* value = user_data.FindDictKey("conversionEnvelope");
  if (!value || !value->is_dict()) {
    return absl::nullopt;
  }

  VerifiableConversionEnvelopeInfo verifiable_conversion_envelope;

  const std::string* algorithm = value->FindStringKey("alg");
  if (algorithm) {
    verifiable_conversion_envelope.algorithm = *algorithm;
  }

  const std::string* ciphertext = value->FindStringKey("ciphertext");
  if (ciphertext) {
    verifiable_conversion_envelope.ciphertext = *ciphertext;
  }

  const std::string* ephemeral_public_key = value->FindStringKey("epk");
  if (ephemeral_public_key) {
    verifiable_conversion_envelope.ephemeral_public_key = *ephemeral_public_key;
  }

  const std::string* nonce = value->FindStringKey("nonce");
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

  std::vector<uint8_t> ciphertext =
      Base64ToBytes(verifiable_conversion_envelope.ciphertext);
  // API requires 16 leading zero-padding bytes
  ciphertext.insert(ciphertext.cbegin(), kCryptoBoxZeroBytes, 0);

  const std::vector<uint8_t> nonce =
      Base64ToBytes(verifiable_conversion_envelope.nonce);
  const std::vector<uint8_t> ephemeral_public_key =
      Base64ToBytes(verifiable_conversion_envelope.ephemeral_public_key);
  const std::vector<uint8_t> advertiser_secret_key =
      Base64ToBytes(advertiser_secret_key_base64);

  const std::vector<uint8_t> plaintext =
      Decrypt(ciphertext, nonce, ephemeral_public_key, advertiser_secret_key);

  const std::string message = (const char*)&plaintext.front();

  return message;
}

absl::optional<std::string> OpenEvenlopeForUserDataAndAdvertiserSecretKey(
    const base::Value& user_data,
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
