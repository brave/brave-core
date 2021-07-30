/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/features.h"

#include "base/feature_list.h"

namespace brave_ads {
namespace features {

const base::Feature kAdNotifications{"AdNotifications",
                                     base::FEATURE_ENABLED_BY_DEFAULT};

namespace {

// Set to true to show custom ad notifications or false to show system
// notifications
const char kFieldTrialParameterShouldShowCustomAdNotifications[] =
    "should_show_custom_notifications";
const bool kDefaultShouldShowCustomAdNotifications = false;

// Ad notification timeout in seconds. Set to 0 to never time out
const char kFieldTrialParameterAdNotificationTimeout[] =
    "ad_notification_timeout";
#if !defined(OS_ANDROID)
const int kDefaultAdNotificationTimeout = 120;
#else
const int kDefaultAdNotificationTimeout = 30;
#endif

#if !defined(OS_ANDROID)

// Ad notification fade animation duration in milliseconds
const char kFieldTrialParameterAdNotificationFadeDuration[] =
    "ad_notification_fade_duration";
const int kDefaultAdNotificationFadeDuration = 200;

// Ad notification normalized display coordinate for the x component should be
// between 0.0 and 1.0; coordinates outside this range will be adjusted to fit
// the work area. Set to 0.0 for left, 0.5 for center or 1.0 for right
const char kFieldTrialParameterAdNotificationNormalizedDisplayCoordinateX[] =
    "ad_notification_normalized_display_coordinate_x";
#if defined(OS_WIN)
const double kDefaultAdNotificationNormalizedDisplayCoordinateX = 1.0;
#elif defined(OS_MAC)
const double kDefaultAdNotificationNormalizedDisplayCoordinateX = 1.0;
#elif defined(OS_LINUX)
const double kDefaultAdNotificationNormalizedDisplayCoordinateX = 1.0;
#endif

// Ad notification x inset within the display's work area specified in screen
// coordinates
const char kFieldTrialParameterAdNotificationInsetX[] =
    "ad_notification_inset_x";
#if defined(OS_WIN)
const int kDefaultAdNotificationInsetX = -13;
#elif defined(OS_MAC)
const int kSystemNotificationWidth = 360;
const int kDefaultAdNotificationInsetX = -(10 + kSystemNotificationWidth);
#elif defined(OS_LINUX)
const int kDefaultAdNotificationInsetX = -13;
#endif

// Ad notification normalized display coordinate for the y component should be
// between 0.0 and 1.0; coordinates outside this range will be adjusted to fit
// the work area. Set to 0.0 for top, 0.5 for middle or 1.0 for bottom
const char kFieldTrialParameterAdNotificationNormalizedDisplayCoordinateY[] =
    "ad_notification_normalized_display_coordinate_y";
#if defined(OS_WIN)
const double kDefaultAdNotificationNormalizedDisplayCoordinateY = 0.0;
#elif defined(OS_MAC)
const double kDefaultAdNotificationNormalizedDisplayCoordinateY = 0.0;
#elif defined(OS_LINUX)
const double kDefaultAdNotificationNormalizedDisplayCoordinateY = 0.0;
#endif

// Ad notification y inset within the display's work area specified in screen
// coordinates
const char kFieldTrialParameterAdNotificationInsetY[] =
    "ad_notification_inset_y";
#if defined(OS_WIN)
const int kDefaultAdNotificationInsetY = 18;
#elif defined(OS_MAC)
const int kDefaultAdNotificationInsetY = 11;
#elif defined(OS_LINUX)
const int kDefaultAdNotificationInsetY = 18;
#endif

#endif  // !defined(OS_ANDROID)

}  // namespace

bool IsAdNotificationsEnabled() {
  return base::FeatureList::IsEnabled(kAdNotifications);
}

bool ShouldShowCustomAdNotifications() {
  return GetFieldTrialParamByFeatureAsBool(
      kAdNotifications, kFieldTrialParameterShouldShowCustomAdNotifications,
      kDefaultShouldShowCustomAdNotifications);
}

int AdNotificationTimeout() {
  return GetFieldTrialParamByFeatureAsInt(
      kAdNotifications, kFieldTrialParameterAdNotificationTimeout,
      kDefaultAdNotificationTimeout);
}

#if !defined(OS_ANDROID)

int AdNotificationFadeDuration() {
  return GetFieldTrialParamByFeatureAsInt(
      kAdNotifications, kFieldTrialParameterAdNotificationFadeDuration,
      kDefaultAdNotificationFadeDuration);
}

double AdNotificationNormalizedDisplayCoordinateX() {
  return GetFieldTrialParamByFeatureAsDouble(
      kAdNotifications,
      kFieldTrialParameterAdNotificationNormalizedDisplayCoordinateX,
      kDefaultAdNotificationNormalizedDisplayCoordinateX);
}

int AdNotificationInsetX() {
  return GetFieldTrialParamByFeatureAsInt(
      kAdNotifications, kFieldTrialParameterAdNotificationInsetX,
      kDefaultAdNotificationInsetX);
}

double AdNotificationNormalizedDisplayCoordinateY() {
  return GetFieldTrialParamByFeatureAsDouble(
      kAdNotifications,
      kFieldTrialParameterAdNotificationNormalizedDisplayCoordinateY,
      kDefaultAdNotificationNormalizedDisplayCoordinateY);
}

int AdNotificationInsetY() {
  return GetFieldTrialParamByFeatureAsInt(
      kAdNotifications, kFieldTrialParameterAdNotificationInsetY,
      kDefaultAdNotificationInsetY);
}

#endif  // !defined(OS_ANDROID)

}  // namespace features
}  // namespace brave_ads
