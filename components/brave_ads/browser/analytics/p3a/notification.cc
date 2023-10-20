/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/analytics/p3a/notification.h"
#include "base/metrics/histogram_macros.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "components/prefs/pref_service.h"

namespace brave_ads {

constexpr char kNotificationPositionHistogramName[] =
    "Brave.Rewards.CustomNotificationPosition";
constexpr base::TimeDelta kRecordPositionDebounceDelay = base::Seconds(5);

void NotificationMetrics::RecordNotificationPositionMetric(
    bool should_show_custom_notification,
    PrefService* profile_prefs) {
  notification_debounce_timer_.Start(
      FROM_HERE, kRecordPositionDebounceDelay,
      base::BindOnce(&NotificationMetrics::OnRecordPositionDebounce,
                     should_show_custom_notification, profile_prefs));
}

void NotificationMetrics::OnRecordPositionDebounce(
    bool should_show_custom_notification,
    PrefService* profile_prefs) {
  bool notification_ads_enabled =
      profile_prefs->GetBoolean(prefs::kOptedInToNotificationAds);
  bool pos_pref_exists = profile_prefs->HasPrefPath(
                             prefs::kNotificationAdLastNormalizedCoordinateX) &&
                         profile_prefs->HasPrefPath(
                             prefs::kNotificationAdLastNormalizedCoordinateY);
  if (!notification_ads_enabled || !should_show_custom_notification ||
      !pos_pref_exists) {
    // If custom notifications are not enabled, or if a custom position
    // is not set, do not report the metric. Suspend it so it's no longer
    // reported.
    UMA_HISTOGRAM_EXACT_LINEAR(kNotificationPositionHistogramName, INT_MAX - 1,
                               9);
    return;
  }
  double x_pos =
      profile_prefs->GetDouble(prefs::kNotificationAdLastNormalizedCoordinateX);
  double y_pos =
      profile_prefs->GetDouble(prefs::kNotificationAdLastNormalizedCoordinateY);
  int answer;
  if (y_pos < 0.33) {
    if (x_pos < 0.33) {
      answer = 1;
    } else if (x_pos < 0.67) {
      answer = 2;
    } else {
      answer = 3;
    }
  } else if (y_pos < 0.67) {
    if (x_pos < 0.33) {
      answer = 4;
    } else if (x_pos < 0.67) {
      // Do not report center of screen.
      answer = INT_MAX - 1;
    } else {
      answer = 5;
    }
  } else {
    if (x_pos < 0.33) {
      answer = 6;
    } else if (x_pos < 0.67) {
      answer = 7;
    } else {
      answer = 8;
    }
  }
  UMA_HISTOGRAM_EXACT_LINEAR(kNotificationPositionHistogramName, answer, 9);
}

}  // namespace brave_ads
