/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/bookmark/bookmark_prefs_service.h"

#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"

BookmarkPrefsService::BookmarkPrefsService(Profile* profile)
    : profile_(profile),
      prefs_(profile->GetPrefs()) {
  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(
      kAlwaysShowBookmarkBarOnNTP,
      base::BindRepeating(&BookmarkPrefsService::OnPreferenceChanged,
                          base::Unretained(this)));
}

BookmarkPrefsService::~BookmarkPrefsService() = default;

void BookmarkPrefsService::OnPreferenceChanged() {
  for (Browser* browser : *BrowserList::GetInstance()) {
    if (profile_->IsSameOrParent(browser->profile())) {
      browser->UpdateBookmarkBarState(
          Browser::BOOKMARK_BAR_STATE_CHANGE_PREF_CHANGE);
    }
  }
}
