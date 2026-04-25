/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/split_view_metrics.h"

#include "brave/components/misc_metrics/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/time_period_storage/monthly_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace misc_metrics {

namespace {

constexpr int kSplitViewUsageBuckets[] = {0, 5, 11, 20};

constexpr base::TimeDelta kUpdateInterval = base::Days(1);

}  // namespace

SplitViewMetrics::SplitViewMetrics(PrefService* local_state)
    : local_state_(local_state) {
  usage_storage_ = std::make_unique<MonthlyStorage>(
      local_state, kMiscMetricsSplitViewUsageStorage);

  ReportMetrics();
}

SplitViewMetrics::~SplitViewMetrics() = default;

void SplitViewMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(kMiscMetricsSplitViewUsageStorage);
}

void SplitViewMetrics::ReportSplitViewUsage() {
  usage_storage_->AddDelta(1);
  ReportMetrics();
}

void SplitViewMetrics::ReportMetrics() {
  uint64_t monthly_usage = usage_storage_->GetMonthlySum();
  p3a_utils::RecordToHistogramBucket(kSplitViewUsageHistogramName,
                                     kSplitViewUsageBuckets, monthly_usage);

  update_timer_.Start(FROM_HERE, base::Time::Now() + kUpdateInterval, this,
                      &SplitViewMetrics::ReportMetrics);
}

}  // namespace misc_metrics
