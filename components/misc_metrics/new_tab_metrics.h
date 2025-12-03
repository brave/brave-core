// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_MISC_METRICS_NEW_TAB_METRICS_H_
#define BRAVE_COMPONENTS_MISC_METRICS_NEW_TAB_METRICS_H_

#include <optional>

#include "base/timer/wall_clock_timer.h"
#include "brave/components/time_period_storage/weekly_storage.h"

class PrefRegistrySimple;
class PrefService;

namespace misc_metrics {

inline constexpr char kNTPSearchEngineHistogramName[] =
    "Brave.Search.WidgetDefault";
inline constexpr char kNTPSearchUsageHistogramName[] =
    "Brave.Search.WidgetUsage";
inline constexpr char kNTPGoogleWidgetUsageHistogramName[] =
    "Brave.Search.GoogleWidgetUsage";

class NewTabMetrics {
 public:
  explicit NewTabMetrics(PrefService* local_state);
  ~NewTabMetrics();

  NewTabMetrics(const NewTabMetrics&) = delete;
  NewTabMetrics& operator=(const NewTabMetrics&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void ReportNTPSearchDefaultEngine(std::optional<int64_t> prepopulate_id);
  void ReportNTPSearchUsage(int64_t prepopulate_id);

 private:
  void ReportCounts();

  WeeklyStorage usage_storage_;

  base::WallClockTimer update_timer_;
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_NEW_TAB_METRICS_H_
