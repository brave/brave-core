/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/speedreader/speedreader_tab_helper.h"

#include "brave/browser/brave_browser_process.h"
#include "brave/browser/speedreader/speedreader_service_factory.h"
#include "brave/browser/ui/brave_browser_window.h"
#include "brave/browser/ui/speedreader/speedreader_bubble_view.h"
#include "brave/components/speedreader/speedreader_extended_info_handler.h"
#include "brave/components/speedreader/speedreader_rewriter_service.h"
#include "brave/components/speedreader/speedreader_service.h"
#include "brave/components/speedreader/speedreader_util.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

namespace speedreader {

SpeedreaderTabHelper::~SpeedreaderTabHelper() {
  HideBubble();
}

base::WeakPtr<SpeedreaderTabHelper> SpeedreaderTabHelper::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

SpeedreaderTabHelper::SpeedreaderTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<SpeedreaderTabHelper>(*web_contents) {}

bool SpeedreaderTabHelper::IsSpeedreaderEnabled() const {
  Profile* profile = Profile::FromBrowserContext(
      const_cast<content::WebContents&>(GetWebContents()).GetBrowserContext());
  DCHECK(profile);
  return SpeedreaderServiceFactory::GetForProfile(profile)->IsEnabled();
}

bool SpeedreaderTabHelper::IsEnabledForSite() {
  return IsEnabledForSite(GetWebContents().GetLastCommittedURL());
}

bool SpeedreaderTabHelper::IsEnabledForSite(const GURL& url) {
  if (!IsSpeedreaderEnabled())
    return false;

  Profile* profile =
      Profile::FromBrowserContext(GetWebContents().GetBrowserContext());
  auto* content_rules = HostContentSettingsMapFactory::GetForProfile(profile);
  return speedreader::IsEnabledForSite(content_rules, url);
}

void SpeedreaderTabHelper::MaybeToggleEnabledForSite(bool on) {
  if (!IsSpeedreaderEnabled())
    return;

  if (auto* entry = GetWebContents().GetController().GetLastCommittedEntry()) {
    SpeedreaderExtendedInfoHandler::ClearPersistedData(entry);
  }

  Profile* profile =
      Profile::FromBrowserContext(GetWebContents().GetBrowserContext());
  auto* content_rules = HostContentSettingsMapFactory::GetForProfile(profile);
  bool enabled = speedreader::IsEnabledForSite(
      content_rules, GetWebContents().GetLastCommittedURL());
  if (enabled != on) {
    speedreader::SetEnabledForSite(content_rules,
                                   GetWebContents().GetLastCommittedURL(), on);
    GetWebContents().GetController().Reload(content::ReloadType::NORMAL, false);
  }
}

void SpeedreaderTabHelper::SingleShotSpeedreader() {
  single_shot_next_request_ = true;

  // Refresh the page so it runs through the speedreader throttle
  auto* contents = &GetWebContents();
  if (contents)
    contents->GetController().Reload(content::ReloadType::NORMAL, false);

  // Determine if bubble should be shown automatically
  Profile* profile =
      Profile::FromBrowserContext(GetWebContents().GetBrowserContext());
  DCHECK(profile);
  auto* speedreader_service = SpeedreaderServiceFactory::GetForProfile(profile);
  if (speedreader_service->ShouldPromptUserToEnable()) {
    ShowReaderModeBubble();
    speedreader_service->IncrementPromptCount();
  }
}

bool SpeedreaderTabHelper::MaybeUpdateCachedState(
    content::NavigationHandle* handle) {
  auto* entry = handle->GetNavigationEntry();
  if (!entry) {
    return false;
  }
  Profile* profile =
      Profile::FromBrowserContext(GetWebContents().GetBrowserContext());
  DCHECK(profile);
  auto* speedreader_service = SpeedreaderServiceFactory::GetForProfile(profile);

  bool cached = false;
  DistillState state =
      SpeedreaderExtendedInfoHandler::GetCachedMode(entry, speedreader_service);
  if (state != DistillState::kUnknown) {
    cached = true;
    distill_state_ = state;
  }
  if (!cached) {
    SpeedreaderExtendedInfoHandler::ClearPersistedData(entry);
  }
  return cached;
}

void SpeedreaderTabHelper::UpdateActiveState(
    content::NavigationHandle* handle) {
  DCHECK(handle);
  DCHECK(handle->IsInMainFrame());

  if (single_shot_next_request_) {
    SetNextRequestState(DistillState::kReaderModePending);
    return;
  }

  // Work only with casual main frame navigations.
  if (handle->GetURL().SchemeIsHTTPOrHTTPS()) {
    auto* rewriter_service =
        g_brave_browser_process->speedreader_rewriter_service();
    if (rewriter_service->URLLooksReadable(handle->GetURL())) {
      VLOG(2) << __func__
              << "URL passed speedreader heuristic: " << handle->GetURL();
      if (!IsSpeedreaderEnabled()) {
        SetNextRequestState(DistillState::kPageProbablyReadable);
      } else if (!IsEnabledForSite(handle->GetURL())) {
        SetNextRequestState(DistillState::kSpeedreaderOnDisabledPage);
      } else {
        SetNextRequestState(DistillState::kSpeedreaderModePending);
      }
      return;
    }
  }
  SetNextRequestState(DistillState::kNone);
}

void SpeedreaderTabHelper::SetNextRequestState(DistillState state) {
  distill_state_ = state;
  single_shot_next_request_ = false;
}

void SpeedreaderTabHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle->IsInMainFrame()) {
    if (!MaybeUpdateCachedState(navigation_handle)) {
      UpdateActiveState(navigation_handle);
    }
  }
}

void SpeedreaderTabHelper::DidRedirectNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle->IsInMainFrame()) {
    if (!MaybeUpdateCachedState(navigation_handle)) {
      UpdateActiveState(navigation_handle);
    }
  }
}

SpeedreaderBubbleView* SpeedreaderTabHelper::speedreader_bubble_view() const {
  return speedreader_bubble_;
}

void SpeedreaderTabHelper::OnBubbleClosed() {
  speedreader_bubble_ = nullptr;
  auto* contents = &GetWebContents();
  Browser* browser = chrome::FindBrowserWithWebContents(contents);
  DCHECK(browser);
  browser->window()->UpdatePageActionIcon(PageActionIconType::kReaderMode);
}

void SpeedreaderTabHelper::ShowSpeedreaderBubble() {
  ShowBubble(true);
}

void SpeedreaderTabHelper::ShowReaderModeBubble() {
  ShowBubble(false);
}

void SpeedreaderTabHelper::ShowBubble(bool is_bubble_speedreader) {
  auto* contents = &GetWebContents();
  Browser* browser = chrome::FindBrowserWithWebContents(contents);
  DCHECK(browser);
  speedreader_bubble_ =
      static_cast<BraveBrowserWindow*>(browser->window())
          ->ShowSpeedreaderBubble(this, is_bubble_speedreader);
  browser->window()->UpdatePageActionIcon(PageActionIconType::kReaderMode);
}

// Hides speedreader information
void SpeedreaderTabHelper::HideBubble() {
  if (speedreader_bubble_) {
    speedreader_bubble_->Hide();
    speedreader_bubble_ = nullptr;
  }
}

void SpeedreaderTabHelper::OnDistillComplete() {
  // Perform a state transition
  if (distill_state_ == DistillState::kSpeedreaderModePending) {
    distill_state_ = DistillState::kSpeedreaderMode;
  } else if (distill_state_ == DistillState::kReaderModePending) {
    distill_state_ = DistillState::kReaderMode;
  } else {
    // We got here via an already cached page.
    DCHECK(distill_state_ == DistillState::kSpeedreaderMode ||
           distill_state_ == DistillState::kReaderMode);
  }
}

void SpeedreaderTabHelper::DidStopLoading() {
  auto* entry = GetWebContents().GetController().GetLastCommittedEntry();
  if (entry) {
    SpeedreaderExtendedInfoHandler::PersistMode(entry, distill_state_);
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(SpeedreaderTabHelper);

}  // namespace speedreader
