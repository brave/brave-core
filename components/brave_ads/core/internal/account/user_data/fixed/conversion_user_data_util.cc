/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/conversion_user_data_util.h"

#include "base/check_op.h"
#include "brave/components/brave_ads/core/internal/account/user_data/fixed/conversion_user_data_constants.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/actions/conversion_action_types.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/actions/conversion_action_types_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/envelope/verifiable_conversion_envelope_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/envelope/verifiable_conversion_envelope_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_info.h"

namespace brave_ads {

base::Value::Dict BuildConversionActionTypeUserData(
    const ConversionInfo& conversion) {
  CHECK_NE(ConversionActionType::kUndefined, conversion.action_type);

  return base::Value::Dict().Set(kConversionActionTypeKey,
                                 ToString(conversion.action_type));
}

std::optional<base::Value::Dict> MaybeBuildVerifiableConversionUserData(
    const ConversionInfo& conversion) {
  if (!UserHasJoinedBraveRewards()) {
    // Do not support verifiable conversions for users who have not joined Brave
    // Rewards.
    return std::nullopt;
  }

  if (!conversion.verifiable) {
    return std::nullopt;
  }

  const std::optional<VerifiableConversionEnvelopeInfo>
      sealed_verifiable_conversion_envelope =
          SealVerifiableConversionEnvelope(*conversion.verifiable);
  if (!sealed_verifiable_conversion_envelope) {
    BLOG(1, "Failed to seal verifiable conversion envelope for id "
                << conversion.verifiable->id << " and "
                << conversion.verifiable->advertiser_public_key_base64
                << " advertiser public key");

    return std::nullopt;
  }

  return base::Value::Dict().Set(
      kVerifiableConversionEnvelopeKey,
      base::Value::Dict()
          .Set(kVerifiableConversionEnvelopeAlgorithmKey,
               GetVerifiableConversionEnvelopeAlgorithm())
          .Set(kVerifiableConversionEnvelopeCipherTextKey,
               sealed_verifiable_conversion_envelope->ciphertext_base64)
          .Set(kVerifiableConversionEnvelopeEphemeralPublicKeyKey,
               sealed_verifiable_conversion_envelope
                   ->ephemeral_key_pair_public_key_base64)
          .Set(kVerifiableConversionEnvelopeNonceKey,
               sealed_verifiable_conversion_envelope->nonce_base64));
}

}  // namespace brave_ads
