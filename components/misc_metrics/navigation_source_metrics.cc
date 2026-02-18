/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/navigation_source_metrics.h"

#include "base/metrics/histogram_macros.h"
#include "brave/components/misc_metrics/page_percentage_metrics.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace misc_metrics {

namespace {

constexpr char kTotalCountKey[] = "total";
constexpr char kTopSitesFavoritesCountKey[] = "top_sites_favorites";
constexpr char kTopSitesFrequentedCountKey[] = "top_sites_frequented";
constexpr char kBookmarksCountKey[] = "bookmarks";
constexpr char kDirectURLCountKey[] = "direct_url";
constexpr char kHistoryCountKey[] = "history";
constexpr char kExternalCountKey[] = "external";

}  // namespace

NavigationSourceMetrics::NavigationSourceMetrics(PrefService* local_state)
    : PagePercentageMetrics(local_state,
                            kMiscMetricsNavSourceCounts,
                            kMiscMetricsNavSourceReportFrameStartTime) {
  ReportNavigationSources();
}

NavigationSourceMetrics::~NavigationSourceMetrics() = default;

// static
void NavigationSourceMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterTimePref(kMiscMetricsNavSourceReportFrameStartTime, {});
  registry->RegisterDictionaryPref(kMiscMetricsNavSourceCounts);
}

void NavigationSourceMetrics::RecordDirectNavigation() {
  IncrementDictCount(kDirectURLCountKey);
}

void NavigationSourceMetrics::RecordHistoryNavigation() {
  IncrementDictCount(kHistoryCountKey);
}

void NavigationSourceMetrics::RecordBookmarkNavigation() {
  IncrementDictCount(kBookmarksCountKey);
}

void NavigationSourceMetrics::RecordTopSiteNavigation(bool is_custom) {
  if (is_custom) {
    IncrementDictCount(kTopSitesFavoritesCountKey);
  } else {
    IncrementDictCount(kTopSitesFrequentedCountKey);
  }
}

void NavigationSourceMetrics::RecordExternalNavigation() {
  IncrementDictCount(kExternalCountKey);
}

void NavigationSourceMetrics::IncrementPagesLoadedCount() {
  IncrementDictCount(kTotalCountKey);
}

void NavigationSourceMetrics::ReportNavigationSources() {
  if (!HasReportIntervalElapsed()) {
    return;
  }

  const base::Value::Dict& counts =
      local_state_->GetDict(kMiscMetricsNavSourceCounts);
  int total = counts.FindInt(kTotalCountKey).value_or(0);

  if (total > 0) {
    RecordPercentageHistogram(
        counts, total, kTopSitesFavoritesCountKey,
        kNavSourceCustomTopSitesSourcePercentHistogramName);
    RecordPercentageHistogram(
        counts, total, kTopSitesFrequentedCountKey,
        kNavSourceFrequentTopSitesSourcePercentHistogramName);
    RecordPercentageHistogram(counts, total, kBookmarksCountKey,
                              kNavSourceBookmarksSourcePercentHistogramName);
    RecordPercentageHistogram(counts, total, kDirectURLCountKey,
                              kNavSourceDirectURLSourcePercentHistogramName);
    RecordPercentageHistogram(counts, total, kHistoryCountKey,
                              kNavSourceHistorySourcePercentHistogramName);
    RecordPercentageHistogram(counts, total, kExternalCountKey,
                              kNavSourceExternalSourcePercentHistogramName);
    UMA_HISTOGRAM_BOOLEAN(kNavSourceNavigatedHistogramName, true);
  }

  ResetCounts();
}

}  // namespace misc_metrics
