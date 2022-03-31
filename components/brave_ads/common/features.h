/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_COMMON_FEATURES_H_

#include <string>

#include "build/build_config.h"

namespace base {
struct Feature;
}  // namespace base

namespace brave_ads {
namespace features {

extern const base::Feature kAdNotifications;
bool IsAdNotificationsEnabled();
bool ShouldSupportMultipleDisplays();
bool CanFallbackToCustomAdNotifications();
bool AllowedToFallbackToCustomAdNotifications();
int AdNotificationTimeout();

extern const base::Feature kCustomAdNotifications;
bool IsCustomAdNotificationsEnabled();
#if !BUILDFLAG(IS_ANDROID)
int AdNotificationFadeDuration();
std::string AdNotificationDarkModeBackgroundColor();
bool ShouldAttachAdNotificationToBrowserWindow();
double AdNotificationNormalizedDisplayCoordinateX();
int AdNotificationInsetX();
double AdNotificationNormalizedDisplayCoordinateY();
int AdNotificationInsetY();
#endif  // !BUILDFLAG(IS_ANDROID)

extern const base::Feature kAllowedToFallbackToCustomAdNotifications;
bool IsAllowedToFallbackToCustomAdNotificationsEnabled();

extern const base::Feature kRequestAdsEnabledApi;
bool IsRequestAdsEnabledApiEnabled();

}  // namespace features
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_COMMON_FEATURES_H_
