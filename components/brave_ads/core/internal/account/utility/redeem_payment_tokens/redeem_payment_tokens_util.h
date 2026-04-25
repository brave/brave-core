/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_PAYMENT_TOKENS_REDEEM_PAYMENT_TOKENS_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_PAYMENT_TOKENS_REDEEM_PAYMENT_TOKENS_UTIL_H_

namespace base {
class Time;
class TimeDelta;
}  // namespace base

namespace brave_ads {

void SetNextTokenRedemptionAt(base::Time next_payment_token_redemption_at);
base::Time ScheduleNextTokenRedemptionAt();

base::TimeDelta CalculateDelayBeforeRedeemingTokens();

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_PAYMENT_TOKENS_REDEEM_PAYMENT_TOKENS_UTIL_H_
