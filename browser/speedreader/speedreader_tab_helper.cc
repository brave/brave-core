/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/speedreader/speedreader_tab_helper.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/speedreader/speedreader_service_factory.h"
#include "brave/components/speedreader/speedreader_service.h"
#include "brave/components/speedreader/speedreader_test_whitelist.h"
#include "brave/components/speedreader/speedreader_whitelist.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/navigation_handle.h"

namespace speedreader {

SpeedreaderTabHelper::~SpeedreaderTabHelper() = default;

SpeedreaderTabHelper::SpeedreaderTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {}

void SpeedreaderTabHelper::UpdateActiveState(
    content::NavigationHandle* handle) {
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  DCHECK(profile);
  const bool enabled =
      SpeedreaderServiceFactory::GetForProfile(profile)->IsEnabled();

  if (!enabled) {
    active_ = false;
    return;
  }

  // Work only with casual main frame navigations.
  if (handle->GetURL().SchemeIsHTTPOrHTTPS() && handle->IsInMainFrame()) {
    auto* whitelist = g_brave_browser_process->speedreader_whitelist();
    if (speedreader::IsWhitelistedForTest(handle->GetURL()) ||
        whitelist->IsWhitelisted(handle->GetURL())) {
      VLOG(2) << __func__ << " SpeedReader active for " << handle->GetURL();
      active_ = true;
      return;
    }
  }
  active_ = false;
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

WEB_CONTENTS_USER_DATA_KEY_IMPL(SpeedreaderTabHelper)

}  // namespace speedreader
