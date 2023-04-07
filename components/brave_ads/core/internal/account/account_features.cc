/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/account_features.h"

#include "base/metrics/field_trial_params.h"

namespace brave_ads::features {

namespace {

constexpr char kNextPaymentDayFieldTrialParamName[] = "next_payment_day";
constexpr int kNextPaymentDayDefaultValue = 7;

}  // namespace

BASE_FEATURE(kAccount, "Account", base::FEATURE_ENABLED_BY_DEFAULT);

bool IsAccountEnabled() {
  return base::FeatureList::IsEnabled(kAccount);
}

int GetNextPaymentDay() {
  return GetFieldTrialParamByFeatureAsInt(kAccount,
                                          kNextPaymentDayFieldTrialParamName,
                                          kNextPaymentDayDefaultValue);
}

}  // namespace brave_ads::features
