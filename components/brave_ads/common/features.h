/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_COMMON_FEATURES_H_

#include <string>

#include "build/build_config.h"  // IWYU pragma: keep

namespace base {
struct Feature;
}  // namespace base

namespace brave_ads::features {

extern const base::Feature kNotificationAds;
bool IsNotificationAdsEnabled();
bool ShouldSupportMultipleDisplays();
bool CanFallbackToCustomNotificationAds();
bool AllowedToFallbackToCustomNotificationAds();
int NotificationAdTimeout();

extern const base::Feature kCustomNotificationAds;
bool IsCustomNotificationAdsEnabled();
#if !BUILDFLAG(IS_ANDROID)
int NotificationAdFadeDuration();
std::string NotificationAdDarkModeBackgroundColor();
bool ShouldAttachNotificationAdToBrowserWindow();
double NotificationAdNormalizedDisplayCoordinateX();
int NotificationAdInsetX();
double NotificationAdNormalizedDisplayCoordinateY();
int NotificationAdInsetY();
#endif  // !BUILDFLAG(IS_ANDROID)

extern const base::Feature kAllowedToFallbackToCustomNotificationAds;
bool IsAllowedToFallbackToCustomNotificationAdsEnabled();

extern const base::Feature kSupportBraveSearchResultAdConfirmationEvents;

}  // namespace brave_ads::features

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_COMMON_FEATURES_H_
