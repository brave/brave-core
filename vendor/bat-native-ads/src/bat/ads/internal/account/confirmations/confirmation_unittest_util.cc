/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/confirmations/confirmation_unittest_util.h"

#include <string>

#include "base/base64url.h"
#include "base/check_op.h"
#include "base/time/time.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/account/account_util.h"
#include "bat/ads/internal/account/confirmations/confirmation_info.h"
#include "bat/ads/internal/account/confirmations/confirmation_payload_json_writer.h"
#include "bat/ads/internal/account/confirmations/confirmation_util.h"
#include "bat/ads/internal/account/confirmations/opted_in_credential_json_writer.h"
#include "bat/ads/internal/account/confirmations/opted_in_info.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/blinded_token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_token_util.h"

namespace ads {

namespace {

absl::optional<OptedInInfo> CreateOptedIn(const ConfirmationInfo& confirmation,
                                          const base::Value::Dict& user_data) {
  DCHECK(ShouldRewardUser());

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
  opted_in.user_data = user_data.Clone();

  // Credential
  ConfirmationInfo new_confirmation = confirmation;
  new_confirmation.opted_in = opted_in;

  const absl::optional<std::string> credential =
      json::writer::WriteOptedInCredential(
          *unblinded_token,
          json::writer::WriteConfirmationPayload(new_confirmation));
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
    const base::Value::Dict& user_data) {
  DCHECK(!created_at.is_null());
  DCHECK(!transaction_id.empty());
  DCHECK(!creative_instance_id.empty());
  DCHECK_NE(ConfirmationType::kUndefined, confirmation_type.value());
  DCHECK_NE(AdType::kUndefined, ad_type.value());

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
      CreateOptedIn(confirmation, user_data);
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

}  // namespace ads
