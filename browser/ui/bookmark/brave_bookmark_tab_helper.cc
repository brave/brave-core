/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/bookmark/brave_bookmark_tab_helper.h"

#include "brave/components/constants/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"

BraveBookmarkTabHelper::BraveBookmarkTabHelper(
    content::WebContents* web_contents)
    : content::WebContentsUserData<BraveBookmarkTabHelper>(*web_contents) {}

BraveBookmarkTabHelper::~BraveBookmarkTabHelper() = default;

void BraveBookmarkTabHelper::AddObserver(BookmarkTabHelperObserver* observer) {
  BookmarkTabHelper::FromWebContents(&GetWebContents())->AddObserver(observer);
}

void BraveBookmarkTabHelper::RemoveObserver(
    BookmarkTabHelperObserver* observer) {
  BookmarkTabHelper::FromWebContents(&GetWebContents())
      ->RemoveObserver(observer);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveBookmarkTabHelper);
