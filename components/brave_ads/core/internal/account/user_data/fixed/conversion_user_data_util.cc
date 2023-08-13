/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/conversion_user_data_util.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/account/user_data/fixed/conversion_user_data_constants.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/conversions/actions/conversion_action_types.h"
#include "brave/components/brave_ads/core/internal/conversions/actions/conversion_action_types_util.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/envelope/verifiable_conversion_envelope_info.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/envelope/verifiable_conversion_envelope_util.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

base::Value::Dict BuildConversionActionTypeUserData(
    const ConversionQueueItemInfo& conversion_queue_item) {
  CHECK_NE(ConversionActionType::kUndefined,
           conversion_queue_item.conversion.action_type);

  return base::Value::Dict().Set(
      kConversionActionTypeKey,
      ConversionActionTypeToString(
          conversion_queue_item.conversion.action_type));
}

absl::optional<base::Value::Dict> MaybeBuildVerifiableConversionUserData(
    const ConversionQueueItemInfo& conversion_queue_item) {
  if (!conversion_queue_item.conversion.verifiable) {
    return absl::nullopt;
  }

  const VerifiableConversionInfo verifiable_conversion =
      *conversion_queue_item.conversion.verifiable;

  const absl::optional<VerifiableConversionEnvelopeInfo>
      sealed_verifiable_conversion_envelope =
          SealVerifiableConversionEnvelope(verifiable_conversion);
  if (!sealed_verifiable_conversion_envelope) {
    BLOG(1, "Failed to seal verifiable conversion envelope for id "
                << verifiable_conversion.id << " and "
                << verifiable_conversion.advertiser_public_key_base64
                << " advertiser public key");
    return absl::nullopt;
  }

  return base::Value::Dict().Set(
      kVerifiableConversionEnvelopeKey,
      base::Value::Dict()
          .Set(kVerifiableConversionEnvelopeAlgorithmKey,
               GetVerifiableConversionEnvelopeAlgorithm())
          .Set(kVerifiableConversionEnvelopeCipherTextKey,
               sealed_verifiable_conversion_envelope->ciphertext)
          .Set(kVerifiableConversionEnvelopeEphemeralPublicKeyKey,
               sealed_verifiable_conversion_envelope->ephemeral_public_key)
          .Set(kVerifiableConversionEnvelopeNonceKey,
               sealed_verifiable_conversion_envelope->nonce));
}

}  // namespace brave_ads
