/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_PRIVACY_HUB_METRICS_H_
#define BRAVE_COMPONENTS_MISC_METRICS_PRIVACY_HUB_METRICS_H_

#include "base/timer/wall_clock_timer.h"

#include "brave/components/time_period_storage/monthly_storage.h"

class PrefRegistrySimple;
class PrefService;

namespace misc_metrics {

extern const char kViewsMonthlyHistogramName[];
extern const char kIsEnabledHistogramName[];

class PrivacyHubMetrics {
 public:
  explicit PrivacyHubMetrics(PrefService* local_state);
  ~PrivacyHubMetrics();

  PrivacyHubMetrics(const PrivacyHubMetrics&) = delete;
  PrivacyHubMetrics& operator=(const PrivacyHubMetrics&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void RecordView();
  void RecordEnabledStatus(bool is_enabled);

 private:
  void RecordViewCount();
  void SetUpTimer();

  MonthlyStorage view_storage_;
  base::WallClockTimer report_timer_;
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_PRIVACY_HUB_METRICS_H_
