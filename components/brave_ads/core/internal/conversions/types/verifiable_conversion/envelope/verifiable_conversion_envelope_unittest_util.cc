/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/envelope/verifiable_conversion_envelope_unittest_util.h"

#include <cstdint>
#include <vector>

#include "base/base64.h"
#include "brave/components/brave_ads/core/internal/common/crypto/crypto_util.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/envelope/verifiable_conversion_envelope_info.h"
#include "tweetnacl.h"  // NOLINT

namespace brave_ads::test {

absl::optional<std::string> OpenVerifiableConversionEnvelope(
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

  // API requires |crypto_box_BOXZEROBYTES| leading zero-padding bytes.
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

}  // namespace brave_ads::test
