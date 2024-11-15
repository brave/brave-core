/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/tab_metrics.h"

#include "base/metrics/histogram_macros.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace misc_metrics {

namespace {
constexpr int kPercentBucketValues[] = {25, 50, 75};
constexpr base::TimeDelta kReportUpdateInterval = base::Days(1);
}  // namespace

TabMetrics::TabMetrics(PrefService* local_state)
    : tab_switcher_new_tabs_storage_(local_state,
                                     kMiscMetricsTabSwitcherNewTabsStorage),
      total_new_tabs_storage_(local_state, kMiscMetricsTotalNewTabsStorage),
      new_tab_location_bar_entries_storage_(
          local_state,
          kMiscMetricsNewTabLocationBarEntriesStorage),
      total_location_bar_entries_storage_(
          local_state,
          kMiscMetricsTotalLocationBarEntriesStorage) {
  UpdateMetrics();
}

TabMetrics::~TabMetrics() = default;

void TabMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(kMiscMetricsTabSwitcherNewTabsStorage);
  registry->RegisterListPref(kMiscMetricsTotalNewTabsStorage);
  registry->RegisterListPref(kMiscMetricsNewTabLocationBarEntriesStorage);
  registry->RegisterListPref(kMiscMetricsTotalLocationBarEntriesStorage);
}

void TabMetrics::RecordLocationBarChange(bool is_new_tab) {
  if (is_new_tab) {
    new_tab_location_bar_entries_storage_.AddDelta(1u);
  }
  total_location_bar_entries_storage_.AddDelta(1u);
  RecordLocationEntries();
}

void TabMetrics::RecordAppMenuNewTab() {
  total_new_tabs_storage_.AddDelta(1);
  RecordNewTabMethods();
}

void TabMetrics::RecordTabSwitcherNewTab() {
  tab_switcher_new_tabs_storage_.AddDelta(1);
  total_new_tabs_storage_.AddDelta(1);
  RecordNewTabMethods();
}

void TabMetrics::RecordLocationEntries() {
  auto total = total_location_bar_entries_storage_.GetWeeklySum();
  if (total == 0) {
    return;
  }

  int percent = static_cast<int>(
      static_cast<double>(
          new_tab_location_bar_entries_storage_.GetWeeklySum()) /
      total * 100);
  p3a_utils::RecordToHistogramBucket(kLocationNewEntriesHistogramName,
                                     kPercentBucketValues, percent);
}

void TabMetrics::RecordNewTabMethods() {
  auto total = total_new_tabs_storage_.GetWeeklySum();
  if (total == 0) {
    return;
  }

  int percent = static_cast<int>(
      static_cast<double>(tab_switcher_new_tabs_storage_.GetWeeklySum()) /
      total * 100);
  p3a_utils::RecordToHistogramBucket(kNewTabMethodsHistogramName,
                                     kPercentBucketValues, percent);
}

void TabMetrics::UpdateMetrics() {
  RecordLocationEntries();
  RecordNewTabMethods();

  SetUpTimer();
}

void TabMetrics::SetUpTimer() {
  report_timer_.Start(
      FROM_HERE, base::Time::Now() + kReportUpdateInterval,
      base::BindOnce(&TabMetrics::UpdateMetrics, base::Unretained(this)));
}

}  // namespace misc_metrics
