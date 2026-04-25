/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_search/browser/backup_results_metrics.h"

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_search/browser/prefs.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/time_period_storage/daily_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_search {

namespace {

constexpr base::TimeDelta kReportUpdateInterval = base::Hours(1);

// Bucket ranges for P3A metric: 0, 1, 2, 3-8, 8+
constexpr int kFailureCountBuckets[] = {0, 1, 2, 8};

}  // namespace

BackupResultsMetrics::BackupResultsMetrics(PrefService* local_state)
    : local_state_(local_state) {
  ReportMetrics();
}

BackupResultsMetrics::~BackupResultsMetrics() = default;

void BackupResultsMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterTimePref(prefs::kBackupResultsLastQueryTime, {});
  registry->RegisterListPref(prefs::kBackupResultsFailuresStorage);
}

void BackupResultsMetrics::RecordQuery(bool is_failure) {
  // Always record the query time when a query is made
  local_state_->SetTime(prefs::kBackupResultsLastQueryTime, base::Time::Now());

  // If it's a failure, increment the failure counter
  if (is_failure) {
    MaybeInitializeFailureStorage();
    failures_storage_->RecordValueNow(1);
  }

  // Update the metric immediately
  ReportMetrics();
}

void BackupResultsMetrics::ReportMetrics() {
  auto now = base::Time::Now();
  auto last_query_time =
      local_state_->GetTime(prefs::kBackupResultsLastQueryTime);

  // Early return if no background search query was made in the past 24 hours
  if (last_query_time.is_null() || (now - last_query_time) >= base::Days(1)) {
    return;
  }

  MaybeInitializeFailureStorage();

  uint64_t failure_count = failures_storage_->GetLast24HourSum();

  p3a_utils::RecordToHistogramBucket(kBackupResultsFailuresHistogramName,
                                     kFailureCountBuckets, failure_count);

  // Set up the next timer after reporting
  report_timer_.Start(FROM_HERE, now + kReportUpdateInterval,
                      base::BindOnce(&BackupResultsMetrics::ReportMetrics,
                                     base::Unretained(this)));
}

void BackupResultsMetrics::MaybeInitializeFailureStorage() {
  if (!failures_storage_) {
    failures_storage_ = std::make_unique<DailyStorage>(
        local_state_, prefs::kBackupResultsFailuresStorage);
  }
}

}  // namespace brave_search
