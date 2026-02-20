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
inline constexpr char kNavSourceExternalSourcePercentHistogramName[] =
    "Brave.Core.ExternalNavigationSourcePercent";
inline constexpr char kNavSourcePWASourcePercentHistogramName[] =
    "Brave.Core.PWASourcePercent";
inline constexpr char kNavSourceNavigatedHistogramName[] =
    "Brave.Core.Navigated";

// Tracks the daily percentage of navigations from each source (top sites,
// bookmarks, direct URL, history, external).
class NavigationSourceMetrics : public PagePercentageMetrics {
 public:
  explicit NavigationSourceMetrics(PrefService* local_state);
  ~NavigationSourceMetrics();

  static void RegisterPrefs(PrefRegistrySimple* registry);

  // Records a directly typed URL navigation.
  void RecordDirectNavigation();
  // Records a history item selected from the omnibox.
  void RecordHistoryNavigation();
  // Records a bookmark navigation (bar, manager, or omnibox selection).
  void RecordBookmarkNavigation();
  // Records a top site tile click. |is_custom| = pinned tile, else frequented.
  void RecordTopSiteNavigation(bool is_custom);
  // Records a navigation opened externally via PAGE_TRANSITION_AUTO_TOPLEVEL.
  void RecordExternalNavigation();
  // Records a navigation that occurred in a PWA (installed app) window.
  void RecordPWANavigation();
  // Increments the total navigation count.
  void IncrementPagesLoadedCount();
  // Reports daily source percentages via P3A.
  void ReportAllMetrics();
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_NAVIGATION_SOURCE_METRICS_H_
