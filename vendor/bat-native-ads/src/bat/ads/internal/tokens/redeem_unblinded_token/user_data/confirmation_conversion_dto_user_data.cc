/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/redeem_unblinded_token/user_data/confirmation_conversion_dto_user_data.h"

#include <utility>

#include "bat/ads/internal/conversions/verifiable_conversion_info.h"
#include "bat/ads/internal/security/conversions/conversions_util.h"
#include "bat/ads/internal/security/conversions/verifiable_conversion_envelope_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace dto {
namespace user_data {

namespace {

const char kAlgorithm[] = "crypto_box_curve25519xsalsa20poly1305";

absl::optional<security::VerifiableConversionEnvelopeInfo> GetEnvelope(
    const ConversionQueueItemInfo& conversion_queue_item) {
  VerifiableConversionInfo verifiable_conversion;
  verifiable_conversion.id = conversion_queue_item.conversion_id;
  verifiable_conversion.public_key =
      conversion_queue_item.advertiser_public_key;

  if (!verifiable_conversion.IsValid()) {
    return absl::nullopt;
  }

  return security::EnvelopeSeal(verifiable_conversion);
}

}  // namespace

base::DictionaryValue GetConversion(
    const ConversionQueueItemInfo& conversion_queue_item) {
  base::DictionaryValue user_data;

  const absl::optional<security::VerifiableConversionEnvelopeInfo> envelope =
      GetEnvelope(conversion_queue_item);
  if (envelope) {
    base::DictionaryValue dictionary;

    dictionary.SetKey("alg", base::Value(kAlgorithm));
    dictionary.SetKey("ciphertext", base::Value(envelope->ciphertext));
    dictionary.SetKey("epk", base::Value(envelope->ephemeral_public_key));
    dictionary.SetKey("nonce", base::Value(envelope->nonce));

    user_data.SetKey("conversionEnvelope", std::move(dictionary));
  }

  return user_data;
}

}  // namespace user_data
}  // namespace dto
}  // namespace ads
