/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_AD_UNITS_NOTIFICATION_AD_NOTIFICATION_AD_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_AD_UNITS_NOTIFICATION_AD_NOTIFICATION_AD_FEATURE_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_constants.h"

namespace brave_ads {

BASE_DECLARE_FEATURE(kNotificationAdFeature);
BASE_DECLARE_FEATURE(kAllowedToFallbackToCustomNotificationAdFeature);

// Ad notification timeout in seconds. Set to 0 to never time out
inline constexpr base::FeatureParam<base::TimeDelta> kNotificationAdTimeout{
    &kNotificationAdFeature, "notification_ad_timeout",
    kDefaultNotificationAdTimeout};

inline constexpr base::FeatureParam<int> kDefaultNotificationAdsPerHour{
    &kNotificationAdFeature, "default_ads_per_hour",
    kDefaultBraveRewardsNotificationAdsPerHour};

// Set to 0 to never cap.
inline constexpr base::FeatureParam<int> kMaximumNotificationAdsPerDay{
    &kNotificationAdFeature, "maximum_ads_per_day", 100};

// Set to true to fallback to custom notification ads if native notifications
// are disabled or false to never fallback
inline constexpr base::FeatureParam<bool> kCanFallbackToCustomNotificationAds{
    &kNotificationAdFeature, "can_fallback_to_custom_notifications",
    kDefaultCanFallbackToCustomNotificationAds};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_AD_UNITS_NOTIFICATION_AD_NOTIFICATION_AD_FEATURE_H_
