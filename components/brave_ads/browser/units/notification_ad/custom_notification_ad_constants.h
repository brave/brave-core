/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_UNITS_NOTIFICATION_AD_CUSTOM_NOTIFICATION_AD_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_UNITS_NOTIFICATION_AD_CUSTOM_NOTIFICATION_AD_CONSTANTS_H_

#include "build/build_config.h"

namespace brave_ads {

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

// Default ad notification fade animation duration in milliseconds
constexpr int kDefaultNotificationAdFadeDuration = 200;

// Default color value is SkColorSetRGB(0x20, 0x23, 0x27);
constexpr char kDefaultNotificationAdDarkModeBackgroundColor[] = "202327";

// Do not support multiple displays by default
constexpr bool kDefaultShouldSupportMultipleDisplays = true;

// Use the same z-order as the browser window by default
constexpr bool kDefaultUseSameZOrderAsBrowserWindow = true;

// Default ad notification normalized display coordinate
#if BUILDFLAG(IS_WIN)
constexpr double kDefaultNotificationAdNormalizedDisplayCoordinateX = 1.0;
#elif BUILDFLAG(IS_MAC)
constexpr double kDefaultNotificationAdNormalizedDisplayCoordinateX = 1.0;
#elif BUILDFLAG(IS_LINUX)
constexpr double kDefaultNotificationAdNormalizedDisplayCoordinateX = 1.0;
#endif

// Default ad notification x inset within the display's work area specified in
// screen coordinates
#if BUILDFLAG(IS_WIN)
constexpr int kDefaultNotificationAdInsetX = -370;
#elif BUILDFLAG(IS_MAC)
constexpr int kNativeNotificationWidth = 360;
constexpr int kDefaultNotificationAdInsetX = -(10 + kNativeNotificationWidth);
#elif BUILDFLAG(IS_LINUX)
constexpr int kDefaultNotificationAdInsetX = -13;
#endif

// Default ad notification normalized display coordinate
#if BUILDFLAG(IS_WIN)
constexpr double kDefaultNotificationAdNormalizedDisplayCoordinateY = 1.0;
#elif BUILDFLAG(IS_MAC)
constexpr double kDefaultNotificationAdNormalizedDisplayCoordinateY = 0.0;
#elif BUILDFLAG(IS_LINUX)
constexpr double kDefaultNotificationAdNormalizedDisplayCoordinateY = 0.0;
#endif

// Default ad notification y inset within the display's work area specified in
// screen coordinates
#if BUILDFLAG(IS_WIN)
constexpr int kDefaultNotificationAdInsetY = -10;
#elif BUILDFLAG(IS_MAC)
constexpr int kDefaultNotificationAdInsetY = 11;
#elif BUILDFLAG(IS_LINUX)
constexpr int kDefaultNotificationAdInsetY = 18;
#endif

#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_UNITS_NOTIFICATION_AD_CUSTOM_NOTIFICATION_AD_CONSTANTS_H_
