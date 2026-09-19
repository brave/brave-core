/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/shared_tab_helper/shared_tab_helper.h"

#include "brave/components/serp_metrics/serp_classifier.h"
#include "brave/components/serp_metrics/serp_classifier_utils.h"
#include "brave/components/serp_metrics/serp_metric_type.h"
#include "brave/components/serp_metrics/serp_metrics.h"

namespace serp_metrics {

namespace {

bool ShouldRecordSearchEngine(SearchEngineType search_engine_type,
                              const GURL& url) {
  // Only Google web searches count. Vertical searches (`tbm` for images, news,
  // video, etc. or a non-zero `udm` for shopping etc.) are excluded.
  return search_engine_type != SEARCH_ENGINE_GOOGLE || IsGoogleWebSearch(url);
}

}  // namespace

SerpMetricsSharedTabHelper::SerpMetricsSharedTabHelper(
    SerpMetrics& serp_metrics)
    : serp_metrics_(serp_metrics) {}

SerpMetricsSharedTabHelper::~SerpMetricsSharedTabHelper() = default;

void SerpMetricsSharedTabHelper::OnNavigationFinished(
    const GURL& url,
    bool is_new_navigation) {
  if (!is_new_navigation || !IsSameSerpAsLastRecorded(url)) {
    // If this isn't a new navigation or it doesn't go to the same SERP as the
    // last recorded one, clear the last recorded SERP URL so the next visit to
    // that SERP can be recorded again.
    last_recorded_serp_url_.reset();
  }

  MaybeClassifyAndRecordSearchEngineForUrl(url);
}

bool SerpMetricsSharedTabHelper::IsSameSerpAsLastRecorded(
    const GURL& url) const {
  return last_recorded_serp_url_ &&
         IsSameSearchQuery(url, *last_recorded_serp_url_);
}

void SerpMetricsSharedTabHelper::MaybeClassifyAndRecordSearchEngineForUrl(
    const GURL& url) {
  if (IsSameSerpAsLastRecorded(url)) {
    return;
  }

  std::optional<SearchEngineType> search_engine_type =
      MaybeClassifySearchEngine(url);
  if (search_engine_type &&
      ShouldRecordSearchEngine(*search_engine_type, url)) {
    RecordSearchEngine(*search_engine_type);
    last_recorded_serp_url_ = url;
  }
}

void SerpMetricsSharedTabHelper::RecordSearchEngine(
    SearchEngineType search_engine_type) {
  switch (search_engine_type) {
    case SEARCH_ENGINE_BRAVE:
      serp_metrics_->RecordSearch(SerpMetricType::kBrave);
      break;

    case SEARCH_ENGINE_GOOGLE:
      serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
      break;

    default:
      // All other search engines are intentionally grouped together.
      serp_metrics_->RecordSearch(SerpMetricType::kOther);
      break;
  }
}

}  // namespace serp_metrics
