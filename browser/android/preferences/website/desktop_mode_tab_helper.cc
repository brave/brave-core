/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/preferences/website/desktop_mode_tab_helper.h"

#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/browser/web_contents/web_contents_impl.h"

namespace {

bool IsDesktopModeEnabled(content::WebContents* contents) {
  PrefService* prefs =
      static_cast<Profile*>(contents->GetBrowserContext())->GetPrefs();
  return prefs->GetBoolean(kDesktopModeEnabled);
}

}  // namespace

DesktopModeTabHelper::DesktopModeTabHelper(content::WebContents* contents)
    : WebContentsObserver(contents) {
}

DesktopModeTabHelper::~DesktopModeTabHelper() {
}

void DesktopModeTabHelper::NavigationEntryCommitted(
    const content::LoadCommittedDetails& load_details) {
  if (!need_override_ua_)
    return;

  const bool desktop_mode_enabled = IsDesktopModeEnabled(web_contents());
  content::NavigationEntry* entry =
      web_contents()->GetController().GetLastCommittedEntry();
  if (desktop_mode_enabled == entry->GetIsOverridingUserAgent())
    return;

  entry->SetIsOverridingUserAgent(desktop_mode_enabled);
  static_cast<content::WebContentsImpl*>(web_contents())->
      UpdateOverridingUserAgent();
}

void DesktopModeTabHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  content::NavigationEntry* entry =
      web_contents()->GetController().GetLastCommittedEntry();
  need_override_ua_ = entry == nullptr;
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(DesktopModeTabHelper);
