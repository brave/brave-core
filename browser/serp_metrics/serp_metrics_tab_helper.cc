/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/serp_metrics/serp_metrics_tab_helper.h"

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/feature_list.h"
#include "brave/browser/misc_metrics/profile_misc_metrics_service.h"
#include "brave/browser/misc_metrics/profile_misc_metrics_service_factory.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/serp_classifier.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/search_engine_type.h"
#include "components/search_engines/search_engine_utils.h"
#include "components/search_engines/template_url_service.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

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

  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());

  if (TemplateURLService* template_url_service =
          TemplateURLServiceFactory::GetForProfile(profile)) {
    serp_classifier_ = std::make_unique<SerpClassifier>(template_url_service);
  } else {
    // `TemplateURLService` can only be null in tests.
    CHECK_IS_TEST();
  }

  misc_metrics::ProfileMiscMetricsService* profile_misc_metrics_service =
      misc_metrics::ProfileMiscMetricsServiceFactory::GetServiceForContext(
          profile);
  CHECK(profile_misc_metrics_service);
  serp_metrics_ = profile_misc_metrics_service->GetSerpMetrics();
  CHECK(serp_metrics_);
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

bool SerpMetricsTabHelper::IsSameSearchQuery(const GURL& url) const {
  if (!serp_classifier_) {
    CHECK_IS_TEST();
    return false;
  }

  return last_recorded_serp_url_ &&
         serp_classifier_->IsSameSearchQuery(url, *last_recorded_serp_url_);
}

void SerpMetricsTabHelper::MaybeClassifyAndRecordSearchEngineForUrl(
    const GURL& url) {
  if (!serp_classifier_) {
    CHECK_IS_TEST();
    return;
  }

  if (IsSameSearchQuery(url)) {
    // The navigation repeats the same search query as the last recorded SERP,
    // so do not double-count it.
    return;
  }

  std::optional<SearchEngineType> search_engine_type =
      serp_classifier_->MaybeClassify(url);
  if (!search_engine_type) {
    return;
  }

  RecordSearchEngine(*search_engine_type);

  last_recorded_serp_url_ = url;
}

void SerpMetricsTabHelper::RecordSearchEngine(
    SearchEngineType search_engine_type) {
  switch (search_engine_type) {
    case SEARCH_ENGINE_BRAVE: {
      serp_metrics_->RecordBraveSearch();
      break;
    }

    case SEARCH_ENGINE_GOOGLE: {
      serp_metrics_->RecordGoogleSearch();
      break;
    }

    default: {
      serp_metrics_->RecordOtherSearch();
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
      !navigation_handle->HasCommitted() ||
      navigation_handle->GetRestoreType() == content::RestoreType::kRestored) {
    return;
  }

  const GURL& url = navigation_handle->GetURL();

  const bool is_new_navigation =
      ui::PageTransitionIsNewNavigation(navigation_handle->GetPageTransition());

  if (!IsSameSearchQuery(url) && is_new_navigation) {
    // Any navigation that doesn't match the previous search engine results page
    // should reset it along with any user initiated navigation (omnibox,
    // bookmarks, etc...) whether it matches or not.
    last_recorded_serp_url_.reset();
  }

  if (!navigation_handle->HasUserGesture() &&
      navigation_handle->IsRendererInitiated()) {
    // Ignore renderer-initiated navigations without a user gesture. These
    // navigations may occur automatically (e.g. redirects or SPA updates) and
    // should not be treated as explicit user actions.
    return;
  }

  if (!is_new_navigation) {
    // Ignore back/forward navigations and reloads, as they are not considered
    // user-initiated search queries.
    return;
  }

  MaybeClassifyAndRecordSearchEngineForUrl(url);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(SerpMetricsTabHelper);

}  // namespace serp_metrics
