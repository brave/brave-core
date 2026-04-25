/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SEARCH_BROWSER_BACKUP_RESULTS_METRICS_H_
#define BRAVE_COMPONENTS_BRAVE_SEARCH_BROWSER_BACKUP_RESULTS_METRICS_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/timer/wall_clock_timer.h"

class PrefRegistrySimple;
class PrefService;
class DailyStorage;

namespace brave_search {

inline constexpr char kBackupResultsFailuresHistogramName[] =
    "Brave.Search.BackupResultsFailures";

// Metrics for tracking background search query failures
// Only reports if at least one background search query was made in the past day
class BackupResultsMetrics {
 public:
  explicit BackupResultsMetrics(PrefService* local_state);
  ~BackupResultsMetrics();

  BackupResultsMetrics(const BackupResultsMetrics&) = delete;
  BackupResultsMetrics& operator=(const BackupResultsMetrics&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  // Called when a background search query is made
  // |is_failure| - true if the query failed, false if it succeeded or started
  void RecordQuery(bool is_failure);

 private:
  void ReportMetrics();
  void MaybeInitializeFailureStorage();

  raw_ptr<PrefService> local_state_;

  std::unique_ptr<DailyStorage> failures_storage_;

  base::WallClockTimer report_timer_;
};

}  // namespace brave_search

#endif  // BRAVE_COMPONENTS_BRAVE_SEARCH_BROWSER_BACKUP_RESULTS_METRICS_H_
