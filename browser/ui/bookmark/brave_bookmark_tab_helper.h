/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BOOKMARK_BRAVE_BOOKMARK_TAB_HELPER_H_
#define BRAVE_BROWSER_UI_BOOKMARK_BRAVE_BOOKMARK_TAB_HELPER_H_

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/bookmarks/bookmark_tab_helper.h"
#include "content/public/browser/web_contents_user_data.h"

class BookmarkTabHelperObserver;

// This proxies BookmarkTabHelper apis that used by Browser.
class BraveBookmarkTabHelper
    : public content::WebContentsUserData<BraveBookmarkTabHelper>{
 public:
  BraveBookmarkTabHelper(const BraveBookmarkTabHelper&) = delete;
  BraveBookmarkTabHelper& operator=(const BraveBookmarkTabHelper&) = delete;
  ~BraveBookmarkTabHelper() override;

  bool ShouldShowBookmarkBar();
  void AddObserver(BookmarkTabHelperObserver* observer);
  void RemoveObserver(BookmarkTabHelperObserver* observer);

 private:
  friend class content::WebContentsUserData<BraveBookmarkTabHelper>;

  explicit BraveBookmarkTabHelper(content::WebContents* web_contents);

  raw_ptr<content::WebContents> web_contents_ = nullptr;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_BROWSER_UI_BOOKMARK_BRAVE_BOOKMARK_TAB_HELPER_H_
