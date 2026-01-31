/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/page_metrics_tab_helper.h"

#include "base/check_is_test.h"
#include "base/metrics/histogram_macros.h"
#include "brave/browser/misc_metrics/profile_misc_metrics_service.h"
#include "brave/browser/misc_metrics/profile_misc_metrics_service_factory.h"
#include "brave/components/misc_metrics/page_metrics.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/reload_type.h"
#include "content/public/browser/restore_type.h"

namespace misc_metrics {

namespace {
constexpr char kBraveSearchHost[] = "search.brave.com";
constexpr char kBraveSearchPath[] = "/search";
}  // namespace

PageMetricsTabHelper::PageMetricsTabHelper(content::WebContents* web_contents)
    : WebContentsObserver(web_contents),
      content::WebContentsUserData<PageMetricsTabHelper>(*web_contents),
      browser_context_(web_contents->GetBrowserContext()) {
  auto* profile = Profile::FromBrowserContext(browser_context_);
  if (!profile) {
    return;
  }
  auto* profile_misc_metrics_service =
      ProfileMiscMetricsServiceFactory::GetServiceForContext(
          profile->GetOriginalProfile());
  if (profile_misc_metrics_service) {
    page_metrics_ = profile_misc_metrics_service->GetPageMetrics();
  }
}

PageMetricsTabHelper::~PageMetricsTabHelper() = default;

void PageMetricsTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!IsRelevantNavigationEvent(navigation_handle)) {
    return;
  }
  const auto& url = navigation_handle->GetURL();
  if (url.host() == kBraveSearchHost && url.path() == kBraveSearchPath) {
    page_metrics_->ReportBraveQuery();
  }
  if (IsPrivateWindowEvent()) {
    UMA_HISTOGRAM_BOOLEAN("Brave.Core.PrivateWindowUsed", true);
    return;
  }
  bool is_reload = false;
  auto reload_type = navigation_handle->GetReloadType();
  if (reload_type == content::ReloadType::NORMAL ||
      reload_type == content::ReloadType::BYPASSING_CACHE) {
    if (navigation_handle->IsRendererInitiated()) {
      // Only record user initiated reloads
      return;
    }
    is_reload = true;
  }
  page_metrics_->IncrementPagesLoadedCount(is_reload);
}

bool PageMetricsTabHelper::IsRelevantNavigationEvent(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->GetURL().SchemeIsHTTPOrHTTPS() ||
      navigation_handle->IsSameDocument() ||
      navigation_handle->GetRestoreType() == content::RestoreType::kRestored ||
      !navigation_handle->HasCommitted() || !page_metrics_) {
    return false;
  }

  if (browser_context_ && browser_context_->IsOffTheRecord() &&
      browser_context_->IsTor()) {
    return false;
  }
  return true;
}

bool PageMetricsTabHelper::IsPrivateWindowEvent() {
  return browser_context_ && browser_context_->IsOffTheRecord();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PageMetricsTabHelper);

}  // namespace misc_metrics
