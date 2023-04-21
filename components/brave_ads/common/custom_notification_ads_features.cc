/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/common/custom_notification_ads_features.h"

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace brave_ads::features {

namespace {

constexpr bool kDefaultCanFallbackToCustomNotificationAds = false;
// Set to true to fallback to custom notification ads if native notifications
// are disabled or false to never fallback
constexpr base::FeatureParam<bool> kCanFallbackToCustomNotificationAds{
    &kCustomNotificationAds, "can_fallback_to_custom_notifications",
    kDefaultCanFallbackToCustomNotificationAds};

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

constexpr int kDefaultNotificationAdFadeDuration = 200;
// Ad notification fade animation duration in milliseconds
constexpr base::FeatureParam<int> kNotificationAdFadeDuration{
    &kCustomNotificationAds, "ad_notification_fade_duration",
    kDefaultNotificationAdFadeDuration};

// Default color value is SkColorSetRGB(0x20, 0x23, 0x27);
constexpr char kDefaultNotificationAdDarkModeBackgroundColor[] = "202327";
// Ad notification dark mode background color
constexpr base::FeatureParam<std::string>
    kNotificationAdDarkModeBackgroundColor{
        &kCustomNotificationAds, "ad_notification_dark_mode_background_color",
        kDefaultNotificationAdDarkModeBackgroundColor};

constexpr bool kDefaultShouldSupportMultipleDisplays = false;
// Set to true to support multiple displays or false to only support the primary
// display
constexpr base::FeatureParam<bool> kShouldSupportMultipleDisplays{
    &kCustomNotificationAds, "should_support_multiple_displays",
    kDefaultShouldSupportMultipleDisplays};

#if !BUILDFLAG(IS_LINUX)
constexpr bool kDefaultShouldAttachNotificationAdToBrowserWindow = false;
constexpr base::FeatureParam<bool> kShouldAttachNotificationAdToBrowserWindow{
    &kCustomNotificationAds, "should_attach_ad_notification_to_browser_window",
    kDefaultShouldAttachNotificationAdToBrowserWindow};
#endif  // !BUILDFLAG(IS_LINUX)

#if BUILDFLAG(IS_WIN)
constexpr double kDefaultNotificationAdNormalizedDisplayCoordinateX = 1.0;
#elif BUILDFLAG(IS_MAC)
constexpr double kDefaultNotificationAdNormalizedDisplayCoordinateX = 1.0;
#elif BUILDFLAG(IS_LINUX)
constexpr double kDefaultNotificationAdNormalizedDisplayCoordinateX = 1.0;
#endif
// Ad notification normalized display coordinate for the x component should be
// between 0.0 and 1.0; coordinates outside this range will be adjusted to fit
// the work area. Set to 0.0 for left, 0.5 for center or 1.0 for right
constexpr base::FeatureParam<double>
    kNotificationAdNormalizedDisplayCoordinateX{
        &kCustomNotificationAds,
        "ad_notification_normalized_display_coordinate_x",
        kDefaultNotificationAdNormalizedDisplayCoordinateX};

#if BUILDFLAG(IS_WIN)
constexpr int kDefaultNotificationAdInsetX = -370;
#elif BUILDFLAG(IS_MAC)
constexpr int kNativeNotificationWidth = 360;
constexpr int kDefaultNotificationAdInsetX = -(10 + kNativeNotificationWidth);
#elif BUILDFLAG(IS_LINUX)
constexpr int kDefaultNotificationAdInsetX = -13;
#endif
// Ad notification x inset within the display's work area specified in screen
// coordinates
constexpr base::FeatureParam<int> kNotificationAdInsetX{
    &kCustomNotificationAds, "ad_notification_inset_x",
    kDefaultNotificationAdInsetX};

#if BUILDFLAG(IS_WIN)
constexpr double kDefaultNotificationAdNormalizedDisplayCoordinateY = 1.0;
#elif BUILDFLAG(IS_MAC)
constexpr double kDefaultNotificationAdNormalizedDisplayCoordinateY = 0.0;
#elif BUILDFLAG(IS_LINUX)
constexpr double kDefaultNotificationAdNormalizedDisplayCoordinateY = 0.0;
#endif
// Ad notification normalized display coordinate for the y component should be
// between 0.0 and 1.0; coordinates outside this range will be adjusted to fit
// the work area. Set to 0.0 for top, 0.5 for middle or 1.0 for bottom
constexpr base::FeatureParam<double>
    kNotificationAdNormalizedDisplayCoordinateY{
        &kCustomNotificationAds,
        "ad_notification_normalized_display_coordinate_y",
        kDefaultNotificationAdNormalizedDisplayCoordinateY};

#if BUILDFLAG(IS_WIN)
constexpr int kDefaultNotificationAdInsetY = -10;
#elif BUILDFLAG(IS_MAC)
constexpr int kDefaultNotificationAdInsetY = 11;
#elif BUILDFLAG(IS_LINUX)
constexpr int kDefaultNotificationAdInsetY = 18;
#endif
// Ad notification y inset within the display's work area specified in screen
// coordinates
constexpr base::FeatureParam<int> kNotificationAdInsetY{
    &kCustomNotificationAds, "ad_notification_inset_y",
    kDefaultNotificationAdInsetY};

#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

}  // namespace

BASE_FEATURE(kCustomNotificationAds,
             "CustomAdNotifications",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kAllowedToFallbackToCustomNotificationAds,
             "AllowedToFallbackToCustomAdNotifications",
             base::FEATURE_ENABLED_BY_DEFAULT);

bool IsCustomNotificationAdsEnabled() {
  return base::FeatureList::IsEnabled(kCustomNotificationAds);
}

bool CanFallbackToCustomNotificationAds() {
  return kCanFallbackToCustomNotificationAds.Get();
}

bool IsAllowedToFallbackToCustomNotificationAdsEnabled() {
  return base::FeatureList::IsEnabled(
      kAllowedToFallbackToCustomNotificationAds);
}

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

int NotificationAdFadeDuration() {
  return kNotificationAdFadeDuration.Get();
}

std::string NotificationAdDarkModeBackgroundColor() {
  return kNotificationAdDarkModeBackgroundColor.Get();
}

bool ShouldSupportMultipleDisplays() {
  return kShouldSupportMultipleDisplays.Get();
}

// TODO(https://github.com/brave/brave-browser/issues/29744): Enable the feature
// parameter for Linux when the attached custom notification ad for Linux is
// implemented.
bool ShouldAttachNotificationAdToBrowserWindow() {
#if !BUILDFLAG(IS_LINUX)
  return kShouldAttachNotificationAdToBrowserWindow.Get();
#else
  return false;
#endif  // !BUILDFLAG(IS_LINUX)
}

double NotificationAdNormalizedDisplayCoordinateX() {
  return kNotificationAdNormalizedDisplayCoordinateX.Get();
}

int NotificationAdInsetX() {
  return kNotificationAdInsetX.Get();
}

double NotificationAdNormalizedDisplayCoordinateY() {
  return kNotificationAdNormalizedDisplayCoordinateY.Get();
}

int NotificationAdInsetY() {
  return kNotificationAdInsetY.Get();
}

#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

}  // namespace brave_ads::features
