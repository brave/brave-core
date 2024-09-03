/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/envelope/verifiable_conversion_envelope_test_util.h"

#include <cstdint>
#include <vector>

#include "base/base64.h"
#include "brave/components/brave_ads/core/internal/common/crypto/crypto_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/envelope/verifiable_conversion_envelope_info.h"
#include "tweetnacl.h"  // NOLINT

namespace brave_ads::test {

std::optional<std::string> OpenVerifiableConversionEnvelope(
    const VerifiableConversionEnvelopeInfo& verifiable_conversion_envelope,
    const std::string& verifiable_conversion_advertiser_secret_key_base64) {
  CHECK(!verifiable_conversion_advertiser_secret_key_base64.empty());

  if (!verifiable_conversion_envelope.IsValid()) {
    return std::nullopt;
  }

  std::optional<std::vector<uint8_t>> ciphertext =
      base::Base64Decode(verifiable_conversion_envelope.ciphertext_base64);
  if (!ciphertext) {
    return std::nullopt;
  }

  // API requires `crypto_box_BOXZEROBYTES` leading zero-padding bytes.
  ciphertext->insert(ciphertext->cbegin(), crypto_box_BOXZEROBYTES, 0);

  const std::optional<std::vector<uint8_t>> nonce =
      base::Base64Decode(verifiable_conversion_envelope.nonce_base64);
  if (!nonce) {
    return std::nullopt;
  }

  const std::optional<std::vector<uint8_t>> ephemeral_public_key =
      base::Base64Decode(
          verifiable_conversion_envelope.ephemeral_key_pair_public_key_base64);
  if (!ephemeral_public_key) {
    return std::nullopt;
  }

  const std::optional<std::vector<uint8_t>>
      verifiable_conversion_advertiser_secret_key = base::Base64Decode(
          verifiable_conversion_advertiser_secret_key_base64);
  if (!verifiable_conversion_advertiser_secret_key) {
    return std::nullopt;
  }

  const std::vector<uint8_t> plaintext =
      crypto::Decrypt(*ciphertext, *nonce, *ephemeral_public_key,
                      *verifiable_conversion_advertiser_secret_key);

  return std::string(reinterpret_cast<const char*>(plaintext.data()));
}

}  // namespace brave_ads::test
