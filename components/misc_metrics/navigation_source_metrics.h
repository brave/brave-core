/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_NAVIGATION_SOURCE_METRICS_H_
#define BRAVE_COMPONENTS_MISC_METRICS_NAVIGATION_SOURCE_METRICS_H_

#include "brave/components/misc_metrics/page_percentage_metrics.h"

class PrefRegistrySimple;
class PrefService;

namespace misc_metrics {

inline constexpr char kNavSourceCustomTopSitesSourcePercentHistogramName[] =
    "Brave.Core.CustomTopSitesSourcePercent";
inline constexpr char kNavSourceFrequentTopSitesSourcePercentHistogramName[] =
    "Brave.Core.FrequentTopSitesSourcePercent";
inline constexpr char kNavSourceBookmarksSourcePercentHistogramName[] =
    "Brave.Core.BookmarksSourcePercent";
inline constexpr char kNavSourceDirectURLSourcePercentHistogramName[] =
    "Brave.Core.DirectURLSourcePercent";
inline constexpr char kNavSourceHistorySourcePercentHistogramName[] =
    "Brave.Core.HistorySourcePercent";

// Tracks the daily percentage of navigations from each source (top sites,
// bookmarks, direct URL, history).
class NavigationSourceMetrics : public PagePercentageMetrics {
 public:
  explicit NavigationSourceMetrics(PrefService* local_state);
  ~NavigationSourceMetrics() override;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  // Records a directly typed URL navigation.
  void RecordDirectNavigation();
  // Records a history item selected from the omnibox.
  void RecordHistoryNavigation();
  // Records a bookmark navigation (bar, manager, or omnibox selection).
  void RecordBookmarkNavigation();
  // Records a top site tile click. |is_custom| = pinned tile, else frequented.
  void RecordTopSiteNavigation(bool is_custom);
  // Increments the total navigation count.
  void IncrementPagesLoadedCount();
  // Reports daily source percentages via P3A.
  void ReportNavigationSources();
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_NAVIGATION_SOURCE_METRICS_H_
