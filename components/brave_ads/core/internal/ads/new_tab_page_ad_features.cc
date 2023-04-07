/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/new_tab_page_ad_features.h"

#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/metrics/field_trial_params_util.h"

namespace brave_ads::new_tab_page_ads::features {

namespace {

constexpr char kMaximumAdsPerHourFieldTrialParamName[] = "maximum_ads_per_hour";
constexpr int kMaximumAdsPerHourDefaultValue = 4;

constexpr char kMaximumAdsPerDayFieldTrialParamName[] = "maximum_ads_per_day";
constexpr int kMaximumAdsPerDayDefaultValue = 20;

constexpr char kMinimumWaitTimeFieldTrialParamName[] = "minimum_wait_time";
constexpr base::TimeDelta kMinimumWaitTimeDefaultValue = base::Minutes(5);

}  // namespace

BASE_FEATURE(kFeature, "NewTabPageAds", base::FEATURE_ENABLED_BY_DEFAULT);

bool IsEnabled() {
  return base::FeatureList::IsEnabled(kFeature);
}

int GetMaximumAdsPerHour() {
  return GetFieldTrialParamByFeatureAsInt(kFeature,
                                          kMaximumAdsPerHourFieldTrialParamName,
                                          kMaximumAdsPerHourDefaultValue);
}

int GetMaximumAdsPerDay() {
  return GetFieldTrialParamByFeatureAsInt(kFeature,
                                          kMaximumAdsPerDayFieldTrialParamName,
                                          kMaximumAdsPerDayDefaultValue);
}

base::TimeDelta GetMinimumWaitTime() {
  return GetFieldTrialParamByFeatureAsTimeDelta(
      kFeature, kMinimumWaitTimeFieldTrialParamName,
      kMinimumWaitTimeDefaultValue);
}

}  // namespace brave_ads::new_tab_page_ads::features
