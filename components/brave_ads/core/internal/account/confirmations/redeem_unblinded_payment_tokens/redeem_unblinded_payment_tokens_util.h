/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_UTIL_H_

namespace base {
class Time;
class TimeDelta;
}  // namespace base

namespace brave_ads {

void SetNextTokenRedemptionAt(base::Time next_token_redemption_at);
base::Time NextTokenRedemptionAt();

bool HasPreviouslyRedeemedTokens();

base::TimeDelta DelayBeforeRedeemingTokens();

bool ShouldHaveRedeemedTokensInThePast();

base::Time CalculateNextTokenRedemptionAt();

base::TimeDelta CalculateDelayBeforeRedeemingTokens();

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_UTIL_H_
