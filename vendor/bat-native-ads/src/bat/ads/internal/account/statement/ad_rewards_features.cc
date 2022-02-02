/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/statement/ad_rewards_features.h"

#include "base/metrics/field_trial_params.h"

namespace ads {
namespace features {

namespace {

const char kFeatureName[] = "AdRewards";
const char kFieldTrialParameterNextPaymentDay[] = "next_payment_day";

const int kDefaultNextPaymentDay = 5;

}  // namespace

const base::Feature kAdRewards{kFeatureName, base::FEATURE_ENABLED_BY_DEFAULT};

bool IsAdRewardsEnabled() {
  return base::FeatureList::IsEnabled(kAdRewards);
}

int GetAdRewardsNextPaymentDay() {
  return GetFieldTrialParamByFeatureAsInt(
      kAdRewards, kFieldTrialParameterNextPaymentDay, kDefaultNextPaymentDay);
}

}  // namespace features
}  // namespace ads
