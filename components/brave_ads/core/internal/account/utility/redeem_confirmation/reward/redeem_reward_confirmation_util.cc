/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/redeem_reward_confirmation_util.h"

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_util.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

namespace brave_ads {

namespace {

base::expected<void, std::string> ShouldAddPaymentToken(
    const PaymentTokenInfo& payment_token) {
  if (PaymentTokenExists(payment_token)) {
    return base::unexpected("Payment token is a duplicate");
  }

  return base::ok();
}

}  // namespace

base::expected<void, std::string> MaybeAddPaymentToken(
    const PaymentTokenInfo& payment_token) {
  auto result = ShouldAddPaymentToken(payment_token);
  if (!result.has_value()) {
    return result;
  }

  AddPaymentTokens({payment_token});

  return base::ok();
}

void LogPaymentTokenStatus() {
  const base::Time next_token_redemption_at =
      GetProfileTimePref(prefs::kNextTokenRedemptionAt);

  BLOG(1, "You have " << PaymentTokenCount()
                      << " payment tokens which will be redeemed "
                      << FriendlyDateAndTime(next_token_redemption_at));
}

}  // namespace brave_ads
