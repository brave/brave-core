/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/serp_metrics/serp_metrics_tab_helper.h"

#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/serp_classifier.h"
#include "brave/components/serp_metrics/serp_metric_type.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "brave/ios/browser/serp_metrics/serp_metrics_service_factory_ios.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/search_engine_type.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/navigation/navigation_context.h"
#include "ios/web/public/web_state.h"
#include "ui/base/page_transition_types.h"

namespace serp_metrics {

namespace {

bool IsAllowedToSendUsagePings() {
  return GetApplicationContext()->GetLocalState()->GetBoolean(
      kStatsReportingEnabled);
}

}  // namespace

// static
void SerpMetricsTabHelper::MaybeCreateForWebState(web::WebState* web_state) {
  ProfileIOS* profile =
      ProfileIOS::FromBrowserState(web_state->GetBrowserState());

  if (SerpMetrics* serp_metrics =
          SerpMetricsServiceFactoryIOS::GetForProfile(profile)) {
    SerpMetricsTabHelper::CreateForWebState(web_state, *serp_metrics);
  }
}

SerpMetricsTabHelper::SerpMetricsTabHelper(web::WebState* web_state,
                                           SerpMetrics& serp_metrics)
    : web_state_(web_state), serp_metrics_(serp_metrics) {
  web_state_->AddObserver(this);
}

SerpMetricsTabHelper::~SerpMetricsTabHelper() {
  if (web_state_) {
    web_state_->RemoveObserver(this);
  }
}

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

  const std::optional<SearchEngineType> search_engine_type =
      MaybeClassifySearchEngine(url);
  if (!search_engine_type) {
    return;
  }

  switch (*search_engine_type) {
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

  last_recorded_serp_url_ = url;
}

void SerpMetricsTabHelper::DidFinishNavigation(
    web::WebState* web_state,
    web::NavigationContext* navigation_context) {
  if (!IsAllowedToSendUsagePings()) {
    // The user has opted out of usage pings, so SERP metrics are not collected.
    return;
  }

  if (!navigation_context->HasCommitted()) {
    return;
  }

  const bool is_new_navigation = ui::PageTransitionIsNewNavigation(
      navigation_context->GetPageTransition());

  const GURL& url = navigation_context->GetUrl();

  if (!is_new_navigation || !IsSameSerpAsLastRecorded(url)) {
    // If this isn't a new navigation or it doesn't go to the same SERP as the
    // last recorded one, clear the last recorded SERP URL so the next visit to
    // that SERP can be recorded again.
    last_recorded_serp_url_.reset();
  }

  MaybeClassifyAndRecordSearchEngineForUrl(url);
}

void SerpMetricsTabHelper::WebStateDestroyed(web::WebState* web_state) {
  web_state_->RemoveObserver(this);
  web_state_ = nullptr;
}

}  // namespace serp_metrics
