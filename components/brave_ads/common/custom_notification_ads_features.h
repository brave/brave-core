/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_COMMON_CUSTOM_NOTIFICATION_ADS_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_COMMON_CUSTOM_NOTIFICATION_ADS_FEATURES_H_

#include <string>

#include "base/feature_list.h"
#include "build/build_config.h"

namespace brave_ads::features {

BASE_DECLARE_FEATURE(kCustomNotificationAds);
BASE_DECLARE_FEATURE(kAllowedToFallbackToCustomNotificationAds);

bool IsCustomNotificationAdsEnabled();

bool CanFallbackToCustomNotificationAds();

bool IsAllowedToFallbackToCustomNotificationAdsEnabled();

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
int NotificationAdFadeDuration();
std::string NotificationAdDarkModeBackgroundColor();
bool ShouldSupportMultipleDisplays();
bool ShouldAttachNotificationAdToBrowserWindow();
double NotificationAdNormalizedDisplayCoordinateX();
int NotificationAdInsetX();
double NotificationAdNormalizedDisplayCoordinateY();
int NotificationAdInsetY();
#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

}  // namespace brave_ads::features

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_COMMON_CUSTOM_NOTIFICATION_ADS_FEATURES_H_
