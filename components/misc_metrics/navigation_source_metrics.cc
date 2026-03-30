/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/navigation_source_metrics.h"

#include "base/metrics/histogram_macros.h"
#include "brave/components/misc_metrics/page_percentage_metrics.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "build/build_config.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace misc_metrics {

namespace {

constexpr char kTotalCountKey[] = "total";
constexpr char kTopSitesFavoritesCountKey[] = "top_sites_favorites";
constexpr char kTopSitesFrequentedCountKey[] = "top_sites_frequented";
constexpr char kBookmarksCountKey[] = "bookmarks";
constexpr char kDirectURLCountKey[] = "direct_url";
constexpr char kHistoryCountKey[] = "history";
constexpr char kExternalCountKey[] = "external";
constexpr char kPwaCountKey[] = "pwa";

}  // namespace

NavigationSourceMetrics::NavigationSourceMetrics(PrefService* local_state)
    : PagePercentageMetrics(local_state,
                            kMiscMetricsNavSourceCounts,
                            kMiscMetricsNavSourceReportFrameStartTime) {
  ReportAllMetrics();
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
#if BUILDFLAG(IS_ANDROID)
  // On Android, NTP tile navigations use PAGE_TRANSITION_AUTO_BOOKMARK, so
  // PageMetricsTabHelper will have already recorded a bookmark navigation for
  // this same event. Subtract it out so the top site is not double-counted.
  {
    ScopedDictPrefUpdate update(local_state_, kMiscMetricsNavSourceCounts);
    int current = update->FindInt(kBookmarksCountKey).value_or(0);
    if (current > 0) {
      update->Set(kBookmarksCountKey, current - 1);
    }
  }
#endif
  if (is_custom) {
    IncrementDictCount(kTopSitesFavoritesCountKey);
  } else {
    IncrementDictCount(kTopSitesFrequentedCountKey);
  }
}

void NavigationSourceMetrics::RecordExternalNavigation() {
  IncrementDictCount(kExternalCountKey);
}

void NavigationSourceMetrics::RecordPWANavigation() {
  IncrementDictCount(kPwaCountKey);
}

void NavigationSourceMetrics::IncrementPagesLoadedCount() {
  IncrementDictCount(kTotalCountKey);
}

void NavigationSourceMetrics::ReportAllMetrics() {
  if (!HasReportIntervalElapsed()) {
    return;
  }

  const base::DictValue& counts =
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
    RecordPercentageHistogram(counts, total, kPwaCountKey,
                              kNavSourcePWASourcePercentHistogramName);
    UMA_HISTOGRAM_BOOLEAN(kNavSourceNavigatedHistogramName, true);
  }

  ResetCounts();
}

}  // namespace misc_metrics
