/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/page_metrics_tab_helper.h"

#include "base/check_is_test.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/misc_metrics/media_session_metrics.h"
#include "brave/browser/misc_metrics/process_misc_metrics.h"
#include "brave/browser/misc_metrics/profile_misc_metrics_service.h"
#include "brave/browser/misc_metrics/profile_misc_metrics_service_factory.h"
#include "brave/components/misc_metrics/navigation_source_metrics.h"
#include "brave/components/misc_metrics/page_metrics.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/reload_type.h"
#include "content/public/browser/restore_type.h"
#include "ui/base/page_transition_types.h"

namespace misc_metrics {

namespace {

constexpr char kYouTubeDomain[] = "youtube.com";

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

  if (auto* process_metrics = g_brave_browser_process->process_misc_metrics()) {
    media_session_metrics_ = process_metrics->media_session_metrics();
  }
}

PageMetricsTabHelper::~PageMetricsTabHelper() = default;

void PageMetricsTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!IsRelevantNavigationEvent(navigation_handle)) {
    return;
  }
  const GURL& current_url = navigation_handle->GetURL();
  const GURL& previous_url =
      navigation_handle->GetPreviousPrimaryMainFrameURL();
  page_metrics_->brave_search_metrics().MaybeRecordBraveQuery(previous_url,
                                                              current_url);
  bool is_otr = IsPrivateWindowEvent();
  if (is_otr) {
    UMA_HISTOGRAM_BOOLEAN("Brave.Core.PrivateWindowUsed", true);
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

  ui::PageTransition transition = navigation_handle->GetPageTransition();
  MaybeRecordNavigationSource(transition, is_reload);

  page_metrics_->IncrementPagesLoadedCount(is_reload, is_otr);
}

void PageMetricsTabHelper::MediaSessionCreated(
    content::MediaSession* media_session) {
  if (!media_session_metrics_) {
    return;
  }
  media_session_ = media_session;
  media_session_metrics_->OnMediaSessionCreated(media_session);
}

void PageMetricsTabHelper::WebContentsDestroyed() {
  if (!media_session_metrics_ || !media_session_) {
    return;
  }
  media_session_metrics_->OnMediaSessionDestroyed(media_session_);
  media_session_ = nullptr;
}

void PageMetricsTabHelper::MaybeRecordNavigationSource(
    ui::PageTransition transition,
    bool is_reload) {
  if (is_reload) {
    return;
  }
  auto& nav_source_metrics = page_metrics_->navigation_source_metrics();
  auto* tab = tabs::TabInterface::MaybeGetFromContents(web_contents());
  auto* browser_window = tab ? tab->GetBrowserWindowInterface() : nullptr;
  if (browser_window &&
      browser_window->GetType() == BrowserWindowInterface::Type::TYPE_APP) {
    nav_source_metrics.RecordPWANavigation();
  } else if (ui::PageTransitionCoreTypeIs(transition,
                                          ui::PAGE_TRANSITION_AUTO_BOOKMARK)) {
    nav_source_metrics.RecordBookmarkNavigation();
  } else if (ui::PageTransitionCoreTypeIs(transition,
                                          ui::PAGE_TRANSITION_AUTO_TOPLEVEL)) {
    nav_source_metrics.RecordExternalNavigation();
  }
}

bool PageMetricsTabHelper::IsRelevantNavigationEvent(
    content::NavigationHandle* navigation_handle) {
  const GURL& url = navigation_handle->GetURL();
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !url.SchemeIsHTTPOrHTTPS() ||
      navigation_handle->GetRestoreType() == content::RestoreType::kRestored ||
      !navigation_handle->HasCommitted() || !page_metrics_) {
    return false;
  }

  // YouTube is a SPA, so navigations within the same document represent
  // distinct page visits and should be counted.
  if (navigation_handle->IsSameDocument() &&
      !base::EndsWith(url.host(), kYouTubeDomain,
                      base::CompareCase::SENSITIVE)) {
    return false;
  }

  if (browser_context_ && browser_context_->IsOffTheRecord() &&
      browser_context_->IsTor()) {
    UMA_HISTOGRAM_BOOLEAN("Brave.Core.TorWindowUsed", true);
    return false;
  }
  return true;
}

bool PageMetricsTabHelper::IsPrivateWindowEvent() {
  return browser_context_ && browser_context_->IsOffTheRecord();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PageMetricsTabHelper);

}  // namespace misc_metrics
