/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_CONFIRMATION_REWARD_REDEEM_REWARD_CONFIRMATION_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_CONFIRMATION_REWARD_REDEEM_REWARD_CONFIRMATION_FEATURE_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"

namespace brave_ads {

BASE_DECLARE_FEATURE(kRedeemRewardConfirmationFeature);

inline constexpr base::FeatureParam<base::TimeDelta> kFetchPaymentTokenAfter{
    &kRedeemRewardConfirmationFeature, "fetch_payment_token_after",
    base::Seconds(15)};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_CONFIRMATION_REWARD_REDEEM_REWARD_CONFIRMATION_FEATURE_H_
