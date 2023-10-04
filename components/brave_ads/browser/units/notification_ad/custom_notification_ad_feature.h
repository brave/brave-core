/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_UNITS_NOTIFICATION_AD_CUSTOM_NOTIFICATION_AD_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_UNITS_NOTIFICATION_AD_CUSTOM_NOTIFICATION_AD_FEATURE_H_

#include <string>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "brave/components/brave_ads/browser/units/notification_ad/custom_notification_ad_constants.h"
#include "build/build_config.h"

namespace brave_ads {

BASE_DECLARE_FEATURE(kCustomNotificationAdFeature);

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

// Ad notification fade animation duration in milliseconds
constexpr base::FeatureParam<int> kCustomNotificationAdFadeDuration{
    &kCustomNotificationAdFeature, "fade_duration",
    kDefaultNotificationAdFadeDuration};

// Ad notification dark mode background color
constexpr base::FeatureParam<std::string>
    kCustomNotificationAdDarkModeBackgroundColor{
        &kCustomNotificationAdFeature, "dark_mode_background_color",
        kDefaultNotificationAdDarkModeBackgroundColor};

// Set to true to support multiple displays or false to only support the primary
// display
constexpr base::FeatureParam<bool> kShouldSupportMultipleDisplays{
    &kCustomNotificationAdFeature, "should_support_multiple_displays",
    kDefaultShouldSupportMultipleDisplays};

// Set to true to use the same z-order as the browser window
constexpr base::FeatureParam<bool> kUseSameZOrderAsBrowserWindow{
    &kCustomNotificationAdFeature, "use_same_z_order_as_browser_window",
    kDefaultUseSameZOrderAsBrowserWindow};

// Ad notification normalized display coordinate for the x component should be
// between 0.0 and 1.0; coordinates outside this range will be adjusted to fit
// the work area. Set to 0.0 for left, 0.5 for center or 1.0 for right
constexpr base::FeatureParam<double>
    kCustomNotificationAdNormalizedDisplayCoordinateX{
        &kCustomNotificationAdFeature, "normalized_display_coordinate_x",
        kDefaultNotificationAdNormalizedDisplayCoordinateX};

// Ad notification x inset within the display's work area specified in screen
// coordinates
constexpr base::FeatureParam<int> kCustomNotificationAdInsetX{
    &kCustomNotificationAdFeature, "inset_x", kDefaultNotificationAdInsetX};

// Ad notification normalized display coordinate for the y component should be
// between 0.0 and 1.0; coordinates outside this range will be adjusted to fit
// the work area. Set to 0.0 for top, 0.5 for middle or 1.0 for bottom
constexpr base::FeatureParam<double>
    kCustomNotificationAdNormalizedDisplayCoordinateY{
        &kCustomNotificationAdFeature, "normalized_display_coordinate_y",
        kDefaultNotificationAdNormalizedDisplayCoordinateY};

// Ad notification y inset within the display's work area specified in screen
// coordinates
constexpr base::FeatureParam<int> kCustomNotificationAdInsetY{
    &kCustomNotificationAdFeature, "inset_y", kDefaultNotificationAdInsetY};

#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_UNITS_NOTIFICATION_AD_CUSTOM_NOTIFICATION_AD_FEATURE_H_
