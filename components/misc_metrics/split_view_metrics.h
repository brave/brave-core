/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_SPLIT_VIEW_METRICS_H_
#define BRAVE_COMPONENTS_MISC_METRICS_SPLIT_VIEW_METRICS_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/timer/wall_clock_timer.h"

class PrefRegistrySimple;
class PrefService;
class MonthlyStorage;

namespace misc_metrics {

inline constexpr char kSplitViewUsageHistogramName[] =
    "Brave.SplitView.UsageMonthly";

class SplitViewMetrics {
 public:
  explicit SplitViewMetrics(PrefService* local_state);
  ~SplitViewMetrics();

  SplitViewMetrics(const SplitViewMetrics&) = delete;
  SplitViewMetrics& operator=(const SplitViewMetrics&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  // Called when a user creates a split view
  void ReportSplitViewUsage();

 private:
  void ReportMetrics();

  raw_ptr<PrefService> local_state_;
  std::unique_ptr<MonthlyStorage> usage_storage_;

  base::WallClockTimer update_timer_;
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_SPLIT_VIEW_METRICS_H_
