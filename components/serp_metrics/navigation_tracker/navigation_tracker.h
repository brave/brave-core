/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_NAVIGATION_TRACKER_NAVIGATION_TRACKER_H_
#define BRAVE_COMPONENTS_SERP_METRICS_NAVIGATION_TRACKER_NAVIGATION_TRACKER_H_

#include <optional>

#include "base/memory/raw_ref.h"
#include "components/search_engines/search_engine_type.h"
#include "url/gurl.h"

namespace serp_metrics {

class SerpMetrics;

// Contains SERP metrics logic shared by platform-specific tab helpers.
class SerpMetricsNavigationTracker {
 public:
  explicit SerpMetricsNavigationTracker(SerpMetrics& serp_metrics);

  SerpMetricsNavigationTracker(const SerpMetricsNavigationTracker&) = delete;
  SerpMetricsNavigationTracker& operator=(const SerpMetricsNavigationTracker&) =
      delete;

  ~SerpMetricsNavigationTracker();

  void OnNavigationFinished(const GURL& url, bool is_new_navigation);

 private:
  bool IsSameSerpAsLastRecorded(const GURL& url) const;
  void MaybeClassifyAndRecordSearchEngineForUrl(const GURL& url);
  void RecordSearchEngine(SearchEngineType search_engine_type);

  const raw_ref<SerpMetrics> serp_metrics_;
  std::optional<GURL> last_recorded_serp_url_;
};

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_NAVIGATION_TRACKER_NAVIGATION_TRACKER_H_
