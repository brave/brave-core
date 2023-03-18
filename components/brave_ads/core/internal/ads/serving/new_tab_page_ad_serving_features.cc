/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/new_tab_page_ad_serving_features.h"

#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/metrics/field_trial_params_util.h"

namespace brave_ads::new_tab_page_ads::features {

namespace {

constexpr char kServingVersionFieldTrialParamName[] = "serving_version";
constexpr int kServingVersionDefaultValue = 2;

constexpr char kMaximumAdsPerHourFieldTrialParamName[] = "maximum_ads_per_hour";
constexpr int kMaximumAdsPerHourDefaultValue = 4;

constexpr char kMaximumAdsPerDayFieldTrialParamName[] = "maximum_ads_per_day";
constexpr int kMaximumAdsPerDayDefaultValue = 20;

constexpr char kMinimumWaitTimeFieldTrialParamName[] = "minimum_wait_time";
constexpr base::TimeDelta kMinimumWaitTimeDefaultValue = base::Minutes(5);

}  // namespace

BASE_FEATURE(kServing, "NewTabPageAdServing", base::FEATURE_ENABLED_BY_DEFAULT);

bool IsServingEnabled() {
  return base::FeatureList::IsEnabled(kServing);
}

int GetServingVersion() {
  return GetFieldTrialParamByFeatureAsInt(kServing,
                                          kServingVersionFieldTrialParamName,
                                          kServingVersionDefaultValue);
}

int GetMaximumAdsPerHour() {
  return GetFieldTrialParamByFeatureAsInt(kServing,
                                          kMaximumAdsPerHourFieldTrialParamName,
                                          kMaximumAdsPerHourDefaultValue);
}

int GetMaximumAdsPerDay() {
  return GetFieldTrialParamByFeatureAsInt(kServing,
                                          kMaximumAdsPerDayFieldTrialParamName,
                                          kMaximumAdsPerDayDefaultValue);
}

base::TimeDelta GetMinimumWaitTime() {
  return GetFieldTrialParamByFeatureAsTimeDelta(
      kServing, kMinimumWaitTimeFieldTrialParamName,
      kMinimumWaitTimeDefaultValue);
}

}  // namespace brave_ads::new_tab_page_ads::features
