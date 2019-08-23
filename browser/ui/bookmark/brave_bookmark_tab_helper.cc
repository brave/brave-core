/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/bookmark/brave_bookmark_tab_helper.h"

#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

BraveBookmarkTabHelper::BraveBookmarkTabHelper(
    content::WebContents* web_contents)
    : web_contents_(web_contents) {
}

BraveBookmarkTabHelper::~BraveBookmarkTabHelper() {
}

void BraveBookmarkTabHelper::AddObserver(BookmarkTabHelperObserver* observer) {
  BookmarkTabHelper::FromWebContents(web_contents_)->AddObserver(observer);
}

void BraveBookmarkTabHelper::RemoveObserver(
    BookmarkTabHelperObserver* observer) {
  BookmarkTabHelper::FromWebContents(web_contents_)->RemoveObserver(observer);
}

bool BraveBookmarkTabHelper::ShouldShowBookmarkBar() {
  BookmarkTabHelper* helper =
      BookmarkTabHelper::FromWebContents(web_contents_);
  if (!helper)
    return false;

  // Originally, bookmark is visible for NTP when bookmarks are non empty.
  // In that case, we want to hide bookmark bar on NTP if user chooses to hide.
  // When bookmark should be hidden, we do not change it.
  bool should_show = helper->ShouldShowBookmarkBar();
  if (should_show) {
    Profile* profile =
        Profile::FromBrowserContext(web_contents_->GetBrowserContext());
    should_show = profile->GetPrefs()->GetBoolean(kAlwaysShowBookmarkBarOnNTP);
  }

  return should_show;
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveBookmarkTabHelper)
