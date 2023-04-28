/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_unittest_util.h"

#include <string>

#include "base/base64url.h"
#include "base/check_op.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/account_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_payload_json_writer.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/opted_in_credential_json_writer.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/opted_in_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/opted_in_user_data_info.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/token.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_token_util.h"

namespace brave_ads {

namespace {

absl::optional<OptedInInfo> CreateOptedIn(
    const ConfirmationInfo& confirmation,
    const OptedInUserDataInfo& opted_in_user_data) {
  CHECK(ShouldRewardUser());

  OptedInInfo opted_in;

  // Token
  opted_in.token = privacy::cbr::Token(
      R"(aXZNwft34oG2JAVBnpYh/ktTOzr2gi0lKosYNczUUz6ZS9gaDTJmU2FHFps9dIq+QoDwjSjctR5v0rRn+dYo+AHScVqFAgJ5t2s4KtSyawW10gk6hfWPQw16Q0+8u5AG)");
  CHECK(opted_in.token.has_value());

  // Blinded token
  opted_in.blinded_token = privacy::cbr::BlindedToken(
      "Ev5JE4/9TZI/5TqyN9JWfJ1To0HBwQw2rWeAPcdjX3Q=");
  CHECK(opted_in.blinded_token.has_value());

  // Unblinded token
  const absl::optional<privacy::UnblindedTokenInfo> unblinded_token =
      privacy::MaybeGetUnblindedToken();
  if (!unblinded_token) {
    BLOG(0, "Failed to get unblinded token");
    return absl::nullopt;
  }

  if (!privacy::RemoveUnblindedToken(*unblinded_token)) {
    BLOG(0, "Failed to remove unblinded token");
    return absl::nullopt;
  }

  opted_in.unblinded_token = *unblinded_token;

  // User data
  opted_in.user_data = opted_in_user_data;

  // Credential
  ConfirmationInfo mutable_confirmation(confirmation);
  mutable_confirmation.opted_in = opted_in;

  const absl::optional<std::string> credential =
      json::writer::WriteOptedInCredential(
          *unblinded_token,
          json::writer::WriteConfirmationPayload(mutable_confirmation));
  if (!credential) {
    BLOG(0, "Failed to create opted-in credential");
    return absl::nullopt;
  }

  std::string credential_base64url;
  base::Base64UrlEncode(*credential,
                        base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                        &credential_base64url);
  opted_in.credential_base64url = credential_base64url;

  return opted_in;
}

absl::optional<ConfirmationInfo> CreateConfirmation(
    const base::Time created_at,
    const std::string& transaction_id,
    const std::string& creative_instance_id,
    const ConfirmationType& confirmation_type,
    const AdType& ad_type,
    const OptedInUserDataInfo& opted_in_user_data) {
  CHECK(!created_at.is_null());
  CHECK(!transaction_id.empty());
  CHECK(!creative_instance_id.empty());
  CHECK_NE(ConfirmationType::kUndefined, confirmation_type);
  CHECK_NE(AdType::kUndefined, ad_type);

  ConfirmationInfo confirmation;
  confirmation.transaction_id = transaction_id;
  confirmation.creative_instance_id = creative_instance_id;
  confirmation.type = confirmation_type;
  confirmation.ad_type = ad_type;
  confirmation.created_at = created_at;

  if (!ShouldRewardUser()) {
    return confirmation;
  }

  const absl::optional<OptedInInfo> opted_in =
      CreateOptedIn(confirmation, opted_in_user_data);
  if (!opted_in) {
    BLOG(0, "Failed to create opted-in");
    return absl::nullopt;
  }
  confirmation.opted_in = opted_in;

  CHECK(IsValid(confirmation));

  return confirmation;
}

}  // namespace

absl::optional<ConfirmationInfo> BuildConfirmation() {
  return CreateConfirmation(
      /*created_at*/ Now(),
      /*transaction_id*/ "8b742869-6e4a-490c-ac31-31b49130098a",
      /*creative_instance_id*/ "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
      ConfirmationType::kViewed, AdType::kNotificationAd, /*user_data*/ {});
}

}  // namespace brave_ads
