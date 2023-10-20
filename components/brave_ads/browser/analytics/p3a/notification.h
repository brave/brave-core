/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ANALYTICS_P3A_NOTIFICATION_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ANALYTICS_P3A_NOTIFICATION_H_

#include "base/timer/timer.h"
class PrefService;

namespace brave_ads {

extern const char kNotificationPositionHistogramName[];

class NotificationMetrics {
 public:
  NotificationMetrics() = default;

  NotificationMetrics(const NotificationMetrics&) = delete;
  NotificationMetrics& operator=(const NotificationMetrics&) = delete;

  void RecordNotificationPositionMetric(bool should_show_custom_notification,
                                        PrefService* profile_prefs);

 private:
  static void OnRecordPositionDebounce(bool should_show_custom_notification,
                                       PrefService* profile_prefs);

  base::OneShotTimer notification_debounce_timer_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ANALYTICS_P3A_NOTIFICATION_H_
