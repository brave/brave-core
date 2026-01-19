/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/serp_metrics/serp_metrics_tab_helper.h"

#include "brave/browser/misc_metrics/profile_misc_metrics_service.h"
#include "brave/browser/misc_metrics/profile_misc_metrics_service_factory.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/search_engine_util.h"
#include "chrome/browser/browser_process.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/search_engine_utils.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace metrics {

namespace {

bool IsAllowedToSendUsagePings() {
  return g_browser_process->local_state()->GetBoolean(kStatsReportingEnabled);
}

}  // namespace

SerpMetricsTabHelper::SerpMetricsTabHelper(
    content::WebContents* const web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<SerpMetricsTabHelper>(*web_contents) {
  auto* profile_misc_metrics_service =
      misc_metrics::ProfileMiscMetricsServiceFactory::GetServiceForContext(
          web_contents->GetBrowserContext());
  if (profile_misc_metrics_service) {
    serp_metrics_ = profile_misc_metrics_service->GetSerpMetrics();
  }
}

SerpMetricsTabHelper::~SerpMetricsTabHelper() = default;

///////////////////////////////////////////////////////////////////////////////

void SerpMetricsTabHelper::Record(SearchEngineType search_engine_type) {
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

// This method is called when a navigation in the main frame or a subframe has
// completed. It indicates that the navigation has finished, but the document
// might still be loading resources.
void SerpMetricsTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!IsAllowedToSendUsagePings()) {
    return;
  }

  if (!navigation_handle->IsInPrimaryMainFrame()) {
    // Ignore navigations not in the primary main frame. Subframe navigations
    // are not top-level user actions.
    return;
  }

  if (!navigation_handle->HasCommitted()) {
    // Ignore navigations that did not fully commit such as aborted, replaced,
    // or error pages. Only committed navigations count as completed actions.
    return;
  }

  if (navigation_handle->GetRestoreType() == content::RestoreType::kRestored) {
    // Ignore navigations restored from session history such as tab restore.
    // These reflect previously saved state and not a new user action.
    return;
  }

  if (!navigation_handle->HasUserGesture() &&
      navigation_handle->IsRendererInitiated()) {
    // Ignore navigations without an explicit user gesture. This avoids
    // processing navigations triggered automatically or by scripts. Some
    // browser initiated navigations return `false` for `HasUserGesture` so we
    // must also check `IsRendererInitiated`. See crbug.com/617904.
    return;
  }

  if (!ui::PageTransitionIsNewNavigation(
          navigation_handle->GetPageTransition())) {
    // Ignore navigations that are not new actions. Back, forward, and reload
    // navigations reuse existing history entries.
    return;
  }

  const GURL& url = navigation_handle->GetURL();
  if (IsSearchEngineResultsPage(url)) {
    const SearchEngineType search_engine_type =
        search_engine_utils::GetEngineType(url);
    Record(search_engine_type);
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(SerpMetricsTabHelper);

}  // namespace metrics
