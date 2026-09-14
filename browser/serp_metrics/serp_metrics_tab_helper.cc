/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/serp_metrics/serp_metrics_tab_helper.h"

#include "base/check.h"
#include "base/check_deref.h"
#include "base/feature_list.h"
#include "brave/browser/serp_metrics/serp_metrics_service_factory.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "brave/components/serp_metrics/serp_metrics_service.h"
#include "brave/components/serp_metrics/shared_tab_helper/shared_tab_helper.h"
#include "chrome/browser/browser_process.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/page_transition_types.h"
#include "components/prefs/pref_service.h"

namespace serp_metrics {

namespace {

bool IsAllowedToSendUsagePings() {
  return g_browser_process->local_state()->GetBoolean(kStatsReportingEnabled);
}

}  // namespace

SerpMetricsTabHelper::~SerpMetricsTabHelper() = default;

// static
void SerpMetricsTabHelper::MaybeCreateForWebContents(
    content::WebContents* web_contents) {
  CHECK(web_contents);
  CHECK(base::FeatureList::IsEnabled(serp_metrics::kSerpMetricsFeature));

  SerpMetricsService* serp_metrics_service =
      SerpMetricsServiceFactory::GetFor(web_contents->GetBrowserContext());
  if (!serp_metrics_service) {
    // `SerpMetricsService` is null for off-the-record profiles and may be null
    // in tests.
    return;
  }

  SerpMetrics* serp_metrics = serp_metrics_service->Get();
  CreateForWebContents(web_contents, CHECK_DEREF(serp_metrics));
}

SerpMetricsTabHelper::SerpMetricsTabHelper(
    content::WebContents* web_contents,
    SerpMetrics& serp_metrics)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<SerpMetricsTabHelper>(*web_contents),
      serp_metrics_(serp_metrics),
      shared_tab_helper_(
          std::make_unique<SerpMetricsSharedTabHelper>(serp_metrics)) {}

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

  shared_tab_helper_->OnNavigationFinished(
      navigation_handle->GetURL(), is_new_navigation);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(SerpMetricsTabHelper);

}  // namespace serp_metrics
