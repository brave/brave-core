/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_AD_UNITS_NOTIFICATION_AD_CUSTOM_NOTIFICATION_AD_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_AD_UNITS_NOTIFICATION_AD_CUSTOM_NOTIFICATION_AD_CONSTANTS_H_

#include "build/build_config.h"

namespace brave_ads {

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

// Default ad notification fade animation duration in milliseconds
inline constexpr int kDefaultNotificationAdFadeDuration = 200;

// Default color value is SkColorSetRGB(0x20, 0x23, 0x27);
inline constexpr char kDefaultNotificationAdDarkModeBackgroundColor[] =
    "202327";

// Do not support multiple displays by default
inline constexpr bool kDefaultShouldSupportMultipleDisplays = true;

// Use the same z-order as the browser window by default
inline constexpr bool kDefaultUseSameZOrderAsBrowserWindow = true;

// Default ad notification margin within the display's work area
#if BUILDFLAG(IS_WIN)
inline constexpr int kDefaultNotificationAdMargin = 12;
#elif BUILDFLAG(IS_MAC)
inline constexpr int kDefaultNotificationAdMargin = 16;
#elif BUILDFLAG(IS_LINUX)
inline constexpr int kDefaultNotificationAdMargin = 12;
#endif

// Default ad notification normalized display x coordinate
#if BUILDFLAG(IS_WIN)
inline constexpr double kDefaultNotificationAdNormalizedCoordinateX = 1.0;
#elif BUILDFLAG(IS_MAC)
inline constexpr double kDefaultNotificationAdNormalizedCoordinateX = 1.0;
#elif BUILDFLAG(IS_LINUX)
inline constexpr double kDefaultNotificationAdNormalizedCoordinateX = 0.5;
#endif

// Default ad notification normalized display y coordinate
#if BUILDFLAG(IS_WIN)
inline constexpr double kDefaultNotificationAdNormalizedCoordinateY = 0.05;
#elif BUILDFLAG(IS_MAC)
inline constexpr double kDefaultNotificationAdNormalizedCoordinateY = 0.2;
#elif BUILDFLAG(IS_LINUX)
inline constexpr double kDefaultNotificationAdNormalizedCoordinateY = 0.0;
#endif

#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_AD_UNITS_NOTIFICATION_AD_CUSTOM_NOTIFICATION_AD_CONSTANTS_H_
