/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/notification_ad_serving_features.h"

#include "base/metrics/field_trial_params.h"
#include "brave/components/brave_ads/common/constants.h"

namespace brave_ads::notification_ads::features {

namespace {

constexpr char kServingVersionFieldTrialParamName[] = "serving_version";
constexpr int kServingVersionDefaultValue = 2;

constexpr char kDefaultAdsPerHourFieldTrialParamName[] = "default_ads_per_hour";
constexpr int kDefaultAdsPerHourDefaultValue = kDefaultNotificationAdsPerHour;

constexpr char kMaximumAdsPerDayFieldTrialParamName[] = "maximum_ads_per_day";
constexpr int kMaximumAdsPerDayDefaultValue = 100;

}  // namespace

BASE_FEATURE(kServing,
             "NotificationAdServing",
             base::FEATURE_ENABLED_BY_DEFAULT);

bool IsServingEnabled() {
  return base::FeatureList::IsEnabled(kServing);
}

int GetServingVersion() {
  return GetFieldTrialParamByFeatureAsInt(kServing,
                                          kServingVersionFieldTrialParamName,
                                          kServingVersionDefaultValue);
}

int GetDefaultAdsPerHour() {
  return GetFieldTrialParamByFeatureAsInt(kServing,
                                          kDefaultAdsPerHourFieldTrialParamName,
                                          kDefaultAdsPerHourDefaultValue);
}

int GetMaximumAdsPerDay() {
  return GetFieldTrialParamByFeatureAsInt(kServing,
                                          kMaximumAdsPerDayFieldTrialParamName,
                                          kMaximumAdsPerDayDefaultValue);
}

}  // namespace brave_ads::notification_ads::features
