/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/common/ad_notifications_features.h"

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace brave_ads::features {

namespace {

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
constexpr int kDefaultNotificationAdTimeout = 120;
#else
constexpr int kDefaultNotificationAdTimeout = 30;
#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

// Ad notification timeout in seconds. Set to 0 to never time out
constexpr base::FeatureParam<int> kNotificationAdTimeout{
    &kAdNotifications, "ad_notification_timeout",
    kDefaultNotificationAdTimeout};

}  // namespace

BASE_FEATURE(kAdNotifications,
             "AdNotifications",
             base::FEATURE_ENABLED_BY_DEFAULT);

bool IsAdsNotificationEnabled() {
  return base::FeatureList::IsEnabled(kAdNotifications);
}

int NotificationAdTimeout() {
  return kNotificationAdTimeout.Get();
}

}  // namespace brave_ads::features
