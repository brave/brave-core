/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/serp_metrics/serp_metrics_tab_helper.h"

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/feature_list.h"
#include "brave/browser/serp_metrics/serp_metrics_service.h"
#include "brave/browser/serp_metrics/serp_metrics_service_factory.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/serp_classifier.h"
#include "brave/components/serp_metrics/serp_classifier_utils.h"
#include "brave/components/serp_metrics/serp_metric_type.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "chrome/browser/browser_process.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/search_engine_utils.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/page_transition_types.h"

namespace serp_metrics {

namespace {

bool IsAllowedToSendUsagePings() {
  return g_browser_process->local_state()->GetBoolean(kStatsReportingEnabled);
}

}  // namespace

SerpMetricsTabHelper::SerpMetricsTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<SerpMetricsTabHelper>(*web_contents) {
  CHECK(base::FeatureList::IsEnabled(serp_metrics::kSerpMetricsFeature));

  SerpMetricsService* serp_metrics_service =
      SerpMetricsServiceFactory::GetFor(web_contents->GetBrowserContext());
  if (serp_metrics_service) {
    serp_metrics_ = serp_metrics_service->Get();
  } else {
    // `SerpMetricsService` can be null in tests.
    CHECK_IS_TEST();
  }
}

// static
void SerpMetricsTabHelper::MaybeCreateForWebContents(
    content::WebContents* web_contents) {
  CHECK(web_contents);

  if (!web_contents->GetBrowserContext()->IsOffTheRecord()) {
    CreateForWebContents(web_contents);
  }
}

SerpMetricsTabHelper::~SerpMetricsTabHelper() = default;

///////////////////////////////////////////////////////////////////////////////

bool SerpMetricsTabHelper::IsSameSerpAsLastRecorded(const GURL& url) const {
  return last_recorded_serp_url_ &&
         IsSameSearchQuery(url, *last_recorded_serp_url_);
}

void SerpMetricsTabHelper::MaybeClassifyAndRecordSearchEngineForUrl(
    const GURL& url) {
  if (IsSameSerpAsLastRecorded(url)) {
    return;
  }

  std::optional<SearchEngineType> search_engine_type =
      MaybeClassifySearchEngine(url);
  if (!search_engine_type) {
    return;
  }

  if (search_engine_type == SEARCH_ENGINE_GOOGLE && !IsGoogleWebSearch(url)) {
    // Only web searches count toward Google metrics. Vertical searches
    // (`tbm` for images, news, video, etc. or a non-zero `udm` for shopping
    // etc.) are excluded.
    return;
  }

  RecordSearchEngine(*search_engine_type);

  last_recorded_serp_url_ = url;
}

void SerpMetricsTabHelper::RecordSearchEngine(
    SearchEngineType search_engine_type) {
  if (!serp_metrics_) {
    // `SerpMetrics` can be null in tests.
    CHECK_IS_TEST();
    return;
  }

  switch (search_engine_type) {
    case SEARCH_ENGINE_BRAVE: {
      serp_metrics_->RecordSearch(SerpMetricType::kBrave);
      break;
    }

    case SEARCH_ENGINE_GOOGLE: {
      serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
      break;
    }

    default: {
      // All other search engines are intentionally grouped together.
      serp_metrics_->RecordSearch(SerpMetricType::kOther);
      break;
    }
  }
}

void SerpMetricsTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!IsAllowedToSendUsagePings()) {
    // The user has opted out of usage pings, so SERP metrics are not collected.
    return;
  }

  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted()) {
    return;
  }

  const bool is_new_navigation =
      ui::PageTransitionIsNewNavigation(navigation_handle->GetPageTransition());

  const GURL& url = navigation_handle->GetURL();

  if (!is_new_navigation || !IsSameSerpAsLastRecorded(url)) {
    // If this isn't a new navigation or it doesn't go to the same SERP as the
    // last recorded one, clear the last recorded SERP URL so the next visit to
    // that SERP can be recorded again.
    last_recorded_serp_url_.reset();
  }

  MaybeClassifyAndRecordSearchEngineForUrl(url);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(SerpMetricsTabHelper);

}  // namespace serp_metrics
