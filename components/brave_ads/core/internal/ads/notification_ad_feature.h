/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_NOTIFICATION_AD_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_NOTIFICATION_AD_FEATURE_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "brave/components/brave_ads/common/constants.h"

namespace brave_ads {

BASE_DECLARE_FEATURE(kNotificationAdFeature);

bool IsNotificationAdFeatureEnabled();

constexpr base::FeatureParam<int> kDefaultNotificationAdsPerHour{
    &kNotificationAdFeature, "default_ads_per_hour",
    kDefaultBraveRewardsNotificationAdsPerHour};

constexpr base::FeatureParam<int> kMaximumNotificationAdsPerDay{
    &kNotificationAdFeature, "maximum_ads_per_day", 100};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_NOTIFICATION_AD_FEATURE_H_
