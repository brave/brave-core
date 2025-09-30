/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_METRICS_H_
#define BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_METRICS_H_

#include "base/timer/wall_clock_timer.h"
#include "brave/components/time_period_storage/monthly_storage.h"

class PrefRegistrySimple;
class PrefService;
class HostContentSettingsMap;

namespace speedreader {

inline constexpr char kSpeedreaderPageViewsHistogramName[] =
    "Brave.Speedreader.PageViews";
inline constexpr char kSpeedreaderEnabledSitesHistogramName[] =
    "Brave.Speedreader.EnabledSites";

class SpeedreaderMetrics {
 public:
  SpeedreaderMetrics(PrefService* local_state,
                     HostContentSettingsMap* host_content_settings_map,
                     bool is_allowed_for_all_readable_sites);
  ~SpeedreaderMetrics();

  SpeedreaderMetrics(const SpeedreaderMetrics&) = delete;
  SpeedreaderMetrics& operator=(const SpeedreaderMetrics&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void RecordPageView();

  void UpdateEnabledSitesMetric(bool is_allowed_for_all_readable_sites);

 private:
  void ReportPageViews();

  MonthlyStorage page_views_storage_;
  raw_ptr<HostContentSettingsMap> host_content_settings_map_;
  raw_ptr<PrefService> local_state_;

  base::WallClockTimer update_timer_;
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_METRICS_H_
