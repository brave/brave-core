/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_search/browser/backup_results_metrics.h"

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_search/browser/prefs.h"
#include "brave/components/p3a_utils/bucket.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_search {

namespace {

constexpr base::TimeDelta kReportUpdateInterval = base::Hours(1);
constexpr base::TimeDelta kReportInterval = base::Days(1);

// Bucket ranges for P3A metric: 0%, 1-25%, 26-50%, 51-75%, 76-100%
constexpr int kFailurePercentageBuckets[] = {0, 25, 50, 75};

}  // namespace

BackupResultsMetrics::BackupResultsMetrics(PrefService* local_state)
    : local_state_(local_state) {
  if (local_state_->GetTime(prefs::kBackupResultsLastReportTime).is_null()) {
    local_state_->SetTime(prefs::kBackupResultsLastReportTime,
                          base::Time::Now());
  }
  ReportMetrics();
}

BackupResultsMetrics::~BackupResultsMetrics() = default;

// static
void BackupResultsMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(prefs::kBackupResultsTotalQueryCount, 0);
  registry->RegisterIntegerPref(prefs::kBackupResultsFailedQueryCount, 0);
  registry->RegisterTimePref(prefs::kBackupResultsLastReportTime, {});
}

// static
void BackupResultsMetrics::RegisterLocalStatePrefsForMigration(
    PrefRegistrySimple* registry) {
  // Added 07/2026
  registry->RegisterTimePref(prefs::kBackupResultsLastQueryTime, {});
  registry->RegisterListPref(prefs::kBackupResultsFailuresStorage);
}

// static
void BackupResultsMetrics::MigrateObsoleteLocalStatePrefs(
    PrefService* local_state) {
  local_state->ClearPref(prefs::kBackupResultsLastQueryTime);
  local_state->ClearPref(prefs::kBackupResultsFailuresStorage);
}

void BackupResultsMetrics::RecordQuery(bool is_failure) {
  local_state_->SetInteger(
      prefs::kBackupResultsTotalQueryCount,
      local_state_->GetInteger(prefs::kBackupResultsTotalQueryCount) + 1);

  if (is_failure) {
    local_state_->SetInteger(
        prefs::kBackupResultsFailedQueryCount,
        local_state_->GetInteger(prefs::kBackupResultsFailedQueryCount) + 1);
  }

  ReportMetrics();
}

void BackupResultsMetrics::ReportMetrics() {
  auto now = base::Time::Now();

  report_timer_.Start(FROM_HERE, now + kReportUpdateInterval,
                      base::BindOnce(&BackupResultsMetrics::ReportMetrics,
                                     base::Unretained(this)));

  auto last_report_time =
      local_state_->GetTime(prefs::kBackupResultsLastReportTime);

  // Only report once a full 24 hour frame has elapsed.
  if ((now - last_report_time) < kReportInterval) {
    return;
  }

  int total_count =
      local_state_->GetInteger(prefs::kBackupResultsTotalQueryCount);

  // Only report if at least one background search query was made in the
  // past reporting frame.
  if (total_count > 0) {
    int failed_count =
        local_state_->GetInteger(prefs::kBackupResultsFailedQueryCount);
    p3a_utils::RecordPercentageHistogram(kBackupResultsFailuresHistogramName,
                                         kFailurePercentageBuckets,
                                         failed_count, total_count);
  }

  local_state_->SetTime(prefs::kBackupResultsLastReportTime, now);
  local_state_->SetInteger(prefs::kBackupResultsTotalQueryCount, 0);
  local_state_->SetInteger(prefs::kBackupResultsFailedQueryCount, 0);
}

}  // namespace brave_search
