/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/speedreader/speedreader_tab_helper.h"

#include "brave/browser/brave_browser_process.h"
#include "brave/browser/speedreader/speedreader_service_factory.h"
#include "brave/browser/ui/brave_browser_window.h"
#include "brave/browser/ui/speedreader/speedreader_bubble_view.h"
#include "brave/components/speedreader/speedreader_rewriter_service.h"
#include "brave/components/speedreader/speedreader_service.h"
#include "brave/components/speedreader/speedreader_test_whitelist.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "content/public/browser/navigation_handle.h"

namespace speedreader {

// static
SpeedreaderTabHelper* SpeedreaderTabHelper::Get(
    content::WebContents* web_contents) {
  SpeedreaderTabHelper::CreateForWebContents(web_contents);
  SpeedreaderTabHelper* tab_helper =
      SpeedreaderTabHelper::FromWebContents(web_contents);
  return tab_helper;
}

SpeedreaderTabHelper::~SpeedreaderTabHelper() {
  HideBubble();
}

SpeedreaderTabHelper::SpeedreaderTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {}

bool SpeedreaderTabHelper::IsSpeedreaderEnabled() const {
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  DCHECK(profile);
  return SpeedreaderServiceFactory::GetForProfile(profile)->IsEnabled();
}

void SpeedreaderTabHelper::UpdateActiveState(
    content::NavigationHandle* handle) {
  DCHECK(handle);
  DCHECK(handle->IsInMainFrame());

  const bool enabled = IsSpeedreaderEnabled();

  if (!enabled) {
    active_ = false;
    return;
  }

  // Work only with casual main frame navigations.
  if (handle->GetURL().SchemeIsHTTPOrHTTPS()) {
    auto* rewriter_service =
        g_brave_browser_process->speedreader_rewriter_service();
    if (speedreader::IsWhitelistedForTest(handle->GetURL()) ||
        rewriter_service->IsWhitelisted(handle->GetURL())) {
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

SpeedreaderBubbleView* SpeedreaderTabHelper::speedreader_bubble_view() const {
  return speedreader_bubble_;
}

void SpeedreaderTabHelper::OnBubbleClosed() {
  speedreader_bubble_ = nullptr;
}

// Displays speedreader information
void SpeedreaderTabHelper::ShowBubble() {
  auto* contents = web_contents();
  Browser* browser = chrome::FindBrowserWithWebContents(contents);
  DCHECK(browser);
  speedreader_bubble_ =
      static_cast<BraveBrowserWindow*>(browser->window())
          ->ShowSpeedreaderBubble(this, IsSpeedreaderEnabled());
}

// Hides speedreader information
void SpeedreaderTabHelper::HideBubble() {
  if (speedreader_bubble_) {
    speedreader_bubble_->Hide();
    speedreader_bubble_ = nullptr;
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(SpeedreaderTabHelper)

}  // namespace speedreader
