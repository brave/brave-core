/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_payment_tokens/redeem_payment_tokens_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_payment_tokens/redeem_payment_tokens_feature.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/random/random_util.h"
#include "brave/components/brave_ads/core/internal/flags/debug/debug_flag_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

namespace brave_ads {

namespace {

constexpr base::TimeDelta kDebugRedeemPaymentTokensAfter = base::Minutes(2);
constexpr base::TimeDelta kMinimumDelayBeforeRedeemingTokens = base::Minutes(1);

base::Time NextTokenRedemptionAt() {
  return GetProfileTimePref(prefs::kNextTokenRedemptionAt);
}

bool HasPreviouslyRedeemedTokens() {
  return !NextTokenRedemptionAt().is_null();
}

base::TimeDelta DelayBeforeRedeemingTokens() {
  return NextTokenRedemptionAt() - base::Time::Now();
}

bool ShouldHaveRedeemedTokensInThePast() {
  return NextTokenRedemptionAt() < base::Time::Now();
}

}  // namespace

void SetNextTokenRedemptionAt(const base::Time next_token_redemption_at) {
  SetProfileTimePref(prefs::kNextTokenRedemptionAt, next_token_redemption_at);
}

base::Time ScheduleNextTokenRedemptionAt() {
  return base::Time::Now() +
         (ShouldDebug() ? kDebugRedeemPaymentTokensAfter
                        : RandTimeDelta(kRedeemPaymentTokensAfter.Get()));
}

base::TimeDelta CalculateDelayBeforeRedeemingTokens() {
  if (!HasPreviouslyRedeemedTokens()) {
    return ScheduleNextTokenRedemptionAt() - base::Time::Now();
  }

  if (ShouldHaveRedeemedTokensInThePast()) {
    return kMinimumDelayBeforeRedeemingTokens;
  }

  const base::TimeDelta delay = DelayBeforeRedeemingTokens();
  if (delay < kMinimumDelayBeforeRedeemingTokens) {
    return kMinimumDelayBeforeRedeemingTokens;
  }

  return delay;
}

}  // namespace brave_ads
