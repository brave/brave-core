/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/serp_metrics/serp_metrics_tab_helper.h"

#include <optional>

#include "brave/browser/brave_browser_process.h"
#include "brave/browser/misc_metrics/process_misc_metrics.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/serp_classifier.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/search_engine_type.h"
#include "components/search_engines/search_engine_utils.h"
#include "components/search_engines/template_url_service.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "net/http/http_response_headers.h"
#include "url/gurl.h"

namespace metrics {

namespace {

constexpr int kHttpResponseCodeClassSuccess = 2;

bool IsAllowedToSendUsagePings() {
  return g_browser_process->local_state()->GetBoolean(kStatsReportingEnabled);
}

}  // namespace

SerpMetricsTabHelper::SerpMetricsTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<SerpMetricsTabHelper>(*web_contents) {
  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());

  TemplateURLService* template_url_service =
      TemplateURLServiceFactory::GetForProfile(profile);
  if (template_url_service) {
    serp_classifier_ = std::make_unique<SerpClassifier>(template_url_service);
  }

  if (misc_metrics::ProcessMiscMetrics* process_misc_metrics =
          g_brave_browser_process->process_misc_metrics()) {
    serp_metrics_ = process_misc_metrics->serp_metrics();
  }
}

SerpMetricsTabHelper::~SerpMetricsTabHelper() = default;

///////////////////////////////////////////////////////////////////////////////

void SerpMetricsTabHelper::MaybeClassifyAndRecordSearchEngineForUrl(
    const GURL& url) {
  if (!serp_classifier_) {
    return;
  }

  if (std::optional<SearchEngineType> search_engine_type =
          serp_classifier_->Classify(url)) {
    MaybeRecordSearchEngine(*search_engine_type);
  }
}

void SerpMetricsTabHelper::MaybeRecordSearchEngine(
    SearchEngineType search_engine_type) {
  if (!serp_metrics_) {
    return;
  }

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
    return;
  }

  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted()) {
    return;
  }

  if (!navigation_handle->HasUserGesture() &&
      navigation_handle->IsRendererInitiated()) {
    return;
  }

  if (navigation_handle->GetRestoreType() == content::RestoreType::kRestored) {
    return;
  }

  if (!ui::PageTransitionIsNewNavigation(
          navigation_handle->GetPageTransition())) {
    return;
  }

  if (const net::HttpResponseHeaders* response_headers =
          navigation_handle->GetResponseHeaders()) {
    const int response_code_class = response_headers->response_code() / 100;
    if (response_code_class != kHttpResponseCodeClassSuccess) {
      return;
    }
  }

  MaybeClassifyAndRecordSearchEngineForUrl(navigation_handle->GetURL());
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(SerpMetricsTabHelper);

}  // namespace metrics
