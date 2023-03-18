/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/statement/ad_rewards_features.h"

#include "base/metrics/field_trial_params.h"

namespace brave_ads::features {

namespace {

constexpr char kNextPaymentDayFieldTrialParamName[] = "next_payment_day";
constexpr int kNextPaymentDayDefaultValue = 7;

}  // namespace

BASE_FEATURE(kAdRewards, "AdRewards", base::FEATURE_ENABLED_BY_DEFAULT);

bool IsAdRewardsEnabled() {
  return base::FeatureList::IsEnabled(kAdRewards);
}

int GetAdRewardsNextPaymentDay() {
  return GetFieldTrialParamByFeatureAsInt(kAdRewards,
                                          kNextPaymentDayFieldTrialParamName,
                                          kNextPaymentDayDefaultValue);
}

}  // namespace brave_ads::features
