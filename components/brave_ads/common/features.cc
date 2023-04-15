/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/common/features.h"

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace brave_ads::features {

BASE_FEATURE(kNotificationAds,
             "AdNotifications",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kCustomNotificationAds,
             "CustomAdNotifications",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kAllowedToFallbackToCustomNotificationAds,
             "AllowedToFallbackToCustomAdNotifications",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kShouldTriggerSearchResultAdEvents,
             "ShouldTriggerSearchResultAdEvents",
             base::FEATURE_DISABLED_BY_DEFAULT);

namespace {

// Set to true to fallback to custom notification ads if native notifications
// are disabled or false to never fallback
constexpr char kFieldTrialParameterCanFallbackToCustomNotificationAds[] =
    "can_fallback_to_custom_notifications";
constexpr bool kDefaultCanFallbackToCustomNotificationAds = false;

// Ad notification timeout in seconds. Set to 0 to never time out
constexpr char kFieldTrialParameterNotificationAdTimeout[] =
    "ad_notification_timeout";
#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
constexpr int kDefaultNotificationAdTimeout = 120;
#else
constexpr int kDefaultNotificationAdTimeout = 30;
#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

// Ad notification fade animation duration in milliseconds
constexpr char kFieldTrialParameterNotificationAdFadeDuration[] =
    "ad_notification_fade_duration";
constexpr int kDefaultNotificationAdFadeDuration = 200;

// Ad notification dark mode background color
constexpr char kFieldTrialParameterNotificationAdDarkModeBackgroundColor[] =
    "ad_notification_dark_mode_background_color";
// Default color value is SkColorSetRGB(0x20, 0x23, 0x27);
constexpr char kDefaultNotificationAdDarkModeBackgroundColor[] = "202327";

// Set to true to support multiple displays or false to only support the primary
// display
constexpr char kFieldTrialParameterShouldSupportMultipleDisplays[] =
    "should_support_multiple_displays";
constexpr bool kDefaultShouldSupportMultipleDisplays = false;

constexpr char kFieldTrialParameterShouldAttachNotificationAdToBrowserWindow[] =
    "should_attach_ad_notification_to_browser_window";
constexpr bool kDefaultShouldAttachNotificationAdToBrowserWindow = false;

// Ad notification normalized display coordinate for the x component should be
// between 0.0 and 1.0; coordinates outside this range will be adjusted to fit
// the work area. Set to 0.0 for left, 0.5 for center or 1.0 for right
constexpr char
    kFieldTrialParameterNotificationAdNormalizedDisplayCoordinateX[] =
        "ad_notification_normalized_display_coordinate_x";
#if BUILDFLAG(IS_WIN)
constexpr double kDefaultNotificationAdNormalizedDisplayCoordinateX = 1.0;
#elif BUILDFLAG(IS_MAC)
constexpr double kDefaultNotificationAdNormalizedDisplayCoordinateX = 1.0;
#elif BUILDFLAG(IS_LINUX)
constexpr double kDefaultNotificationAdNormalizedDisplayCoordinateX = 1.0;
#endif

// Ad notification x inset within the display's work area specified in screen
// coordinates
constexpr char kFieldTrialParameterNotificationAdInsetX[] =
    "ad_notification_inset_x";
#if BUILDFLAG(IS_WIN)
constexpr int kDefaultNotificationAdInsetX = -370;
#elif BUILDFLAG(IS_MAC)
constexpr int kNativeNotificationWidth = 360;
constexpr int kDefaultNotificationAdInsetX = -(10 + kNativeNotificationWidth);
#elif BUILDFLAG(IS_LINUX)
constexpr int kDefaultNotificationAdInsetX = -13;
#endif

// Ad notification normalized display coordinate for the y component should be
// between 0.0 and 1.0; coordinates outside this range will be adjusted to fit
// the work area. Set to 0.0 for top, 0.5 for middle or 1.0 for bottom
constexpr char
    kFieldTrialParameterNotificationAdNormalizedDisplayCoordinateY[] =
        "ad_notification_normalized_display_coordinate_y";
#if BUILDFLAG(IS_WIN)
constexpr double kDefaultNotificationAdNormalizedDisplayCoordinateY = 1.0;
#elif BUILDFLAG(IS_MAC)
constexpr double kDefaultNotificationAdNormalizedDisplayCoordinateY = 0.0;
#elif BUILDFLAG(IS_LINUX)
constexpr double kDefaultNotificationAdNormalizedDisplayCoordinateY = 0.0;
#endif

// Ad notification y inset within the display's work area specified in screen
// coordinates
constexpr char kFieldTrialParameterNotificationAdInsetY[] =
    "ad_notification_inset_y";
#if BUILDFLAG(IS_WIN)
constexpr int kDefaultNotificationAdInsetY = -10;
#elif BUILDFLAG(IS_MAC)
constexpr int kDefaultNotificationAdInsetY = 11;
#elif BUILDFLAG(IS_LINUX)
constexpr int kDefaultNotificationAdInsetY = 18;
#endif

#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

}  // namespace

bool IsNotificationAdsEnabled() {
  return base::FeatureList::IsEnabled(kNotificationAds);
}

bool CanFallbackToCustomNotificationAds() {
  return GetFieldTrialParamByFeatureAsBool(
      kNotificationAds, kFieldTrialParameterCanFallbackToCustomNotificationAds,
      kDefaultCanFallbackToCustomNotificationAds);
}

int NotificationAdTimeout() {
  return GetFieldTrialParamByFeatureAsInt(
      kNotificationAds, kFieldTrialParameterNotificationAdTimeout,
      kDefaultNotificationAdTimeout);
}

bool IsCustomNotificationAdsEnabled() {
  return base::FeatureList::IsEnabled(kCustomNotificationAds);
}

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

int NotificationAdFadeDuration() {
  return GetFieldTrialParamByFeatureAsInt(
      kCustomNotificationAds, kFieldTrialParameterNotificationAdFadeDuration,
      kDefaultNotificationAdFadeDuration);
}

std::string NotificationAdDarkModeBackgroundColor() {
  std::string param_value = GetFieldTrialParamValueByFeature(
      kCustomNotificationAds,
      kFieldTrialParameterNotificationAdDarkModeBackgroundColor);

  if (param_value.empty()) {
    return kDefaultNotificationAdDarkModeBackgroundColor;
  }

  return param_value;
}

bool ShouldSupportMultipleDisplays() {
  return GetFieldTrialParamByFeatureAsBool(
      kCustomNotificationAds, kFieldTrialParameterShouldSupportMultipleDisplays,
      kDefaultShouldSupportMultipleDisplays);
}

bool ShouldAttachNotificationAdToBrowserWindow() {
  return GetFieldTrialParamByFeatureAsBool(
      kCustomNotificationAds,
      kFieldTrialParameterShouldAttachNotificationAdToBrowserWindow,
      kDefaultShouldAttachNotificationAdToBrowserWindow);
}

double NotificationAdNormalizedDisplayCoordinateX() {
  return GetFieldTrialParamByFeatureAsDouble(
      kCustomNotificationAds,
      kFieldTrialParameterNotificationAdNormalizedDisplayCoordinateX,
      kDefaultNotificationAdNormalizedDisplayCoordinateX);
}

int NotificationAdInsetX() {
  return GetFieldTrialParamByFeatureAsInt(
      kCustomNotificationAds, kFieldTrialParameterNotificationAdInsetX,
      kDefaultNotificationAdInsetX);
}

double NotificationAdNormalizedDisplayCoordinateY() {
  return GetFieldTrialParamByFeatureAsDouble(
      kCustomNotificationAds,
      kFieldTrialParameterNotificationAdNormalizedDisplayCoordinateY,
      kDefaultNotificationAdNormalizedDisplayCoordinateY);
}

int NotificationAdInsetY() {
  return GetFieldTrialParamByFeatureAsInt(
      kCustomNotificationAds, kFieldTrialParameterNotificationAdInsetY,
      kDefaultNotificationAdInsetY);
}

#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

bool IsAllowedToFallbackToCustomNotificationAdsEnabled() {
  return base::FeatureList::IsEnabled(
      kAllowedToFallbackToCustomNotificationAds);
}

}  // namespace brave_ads::features
