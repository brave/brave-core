/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/speedreader/speedreader_tab_helper.h"

#include <memory>

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/speedreader/speedreader_service_factory.h"
#include "brave/components/speedreader/speedreader_rewriter_service.h"
#include "brave/components/speedreader/speedreader_service.h"
#include "brave/components/speedreader/speedreader_test_whitelist.h"
#include "chrome/browser/dom_distiller/dom_distiller_service_factory.h"
#include "chrome/browser/dom_distiller/tab_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "components/dom_distiller/content/browser/distiller_page_web_contents.h"
#include "components/dom_distiller/content/browser/uma_helper.h"
#include "components/dom_distiller/core/url_utils.h"
#include "content/public/browser/navigation_handle.h"

using dom_distiller::UMAHelper;
using dom_distiller::url_utils::IsDistilledPage;

namespace speedreader {

SpeedreaderTabHelper::~SpeedreaderTabHelper() = default;

SpeedreaderTabHelper::SpeedreaderTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {}

bool SpeedreaderTabHelper::IsReaderEnabled() const {
  if (speedreader_active_)
    return true;
  auto url = web_contents()->GetURL();
  return IsDistilledPage(url);
}

void SpeedreaderTabHelper::UpdateActiveState(
    content::NavigationHandle* handle) {
  DCHECK(handle);
  DCHECK(handle->IsInMainFrame());

  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  DCHECK(profile);
  const bool enabled =
      SpeedreaderServiceFactory::GetForProfile(profile)->IsEnabled();
  if (!enabled) {
    speedreader_active_ = false;
    return;
  }

  // Work only with casual main frame navigations.
  if (handle->GetURL().SchemeIsHTTPOrHTTPS()) {
    auto* rewriter_service =
        g_brave_browser_process->speedreader_rewriter_service();
    if (speedreader::IsWhitelistedForTest(handle->GetURL()) ||
        rewriter_service->IsWhitelisted(handle->GetURL())) {
      VLOG(2) << __func__ << " SpeedReader active for " << handle->GetURL();
      speedreader_active_ = true;
      dom_distiller::RemoveObserver(web_contents(), this);
      return;
    }
  }
  speedreader_active_ = false;
  dom_distiller::AddObserver(web_contents(), this);
}

void SpeedreaderTabHelper::WebContentsDestroyed() {
  dom_distiller::RemoveObserver(web_contents(), this);
}

void SpeedreaderTabHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle->IsInMainFrame()) {
    UpdateActiveState(navigation_handle);
  }
}

void SpeedreaderTabHelper::DidRedirectNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle->IsInMainFrame()) {
    UpdateActiveState(navigation_handle);
  }
}

void SpeedreaderTabHelper::OnResult(
    const dom_distiller::DistillabilityResult& result) {
  auto url = web_contents()->GetURL();
  if (result.is_last && result.is_distillable && !IsDistilledPage(url)) {
    std::unique_ptr<dom_distiller::SourcePageHandleWebContents> handle =
        std::make_unique<dom_distiller::SourcePageHandleWebContents>(
            web_contents(), false);
    // We don't care about this timer, but a CHECK fails if not
    // started when distilling.
    UMAHelper::StartTimerIfNeeded(web_contents(),
                                  UMAHelper::ReaderModePageType::kDistillable);
    DistillCurrentPageAndView(handle->web_contents());
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(SpeedreaderTabHelper)

}  // namespace speedreader
