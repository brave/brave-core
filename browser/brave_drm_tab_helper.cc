/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_drm_tab_helper.h"

#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/navigation_handle.h"

#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT) || BUILDFLAG(BUNDLE_WIDEVINE_CDM)
#include "brave/browser/widevine/widevine_utils.h"
#endif

BraveDrmTabHelper::BraveDrmTabHelper(content::WebContents* contents)
    : WebContentsObserver(contents), bindings_(contents, this) {}

BraveDrmTabHelper::~BraveDrmTabHelper() {}

bool BraveDrmTabHelper::ShouldShowWidevineOptIn() const {
  // If the user already opted in, don't offer it.
  PrefService* prefs = ProfileManager::GetActiveUserProfile()->GetPrefs();
  if (prefs->GetBoolean(kWidevineOptedIn)) {
    return false;
  }

  return is_widevine_requested_;
}

void BraveDrmTabHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }
  is_widevine_requested_ = false;
}

void BraveDrmTabHelper::OnWidevineKeySystemAccessRequest() {
  is_widevine_requested_ = true;

  Browser* browser = chrome::FindBrowserWithWebContents(web_contents());
  browser->window()->UpdateToolbar(web_contents());

#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT) || BUILDFLAG(BUNDLE_WIDEVINE_CDM)
  // Show permission request bubble because showing blocked image alone in
  // omnibox is not obvious to the user and easy to miss.
  if (ShouldShowWidevineOptIn() && !is_widevine_permission_requested_) {
    RequestWidevinePermission(web_contents());
    is_widevine_permission_requested_ = true;
  }
#endif
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveDrmTabHelper)
