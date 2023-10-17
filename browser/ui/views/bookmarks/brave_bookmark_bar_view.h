
/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BOOKMARKS_BRAVE_BOOKMARK_BAR_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BOOKMARKS_BRAVE_BOOKMARK_BAR_VIEW_H_

#include "chrome/browser/ui/views/bookmarks/bookmark_bar_view.h"
#include "components/prefs/pref_member.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BraveBookmarkBarView : public BookmarkBarView {
  METADATA_HEADER(BraveBookmarkBarView, BookmarkBarView)
 public:

  BraveBookmarkBarView(Browser* browser, BrowserView* browser_view);
  ~BraveBookmarkBarView() override;

  // BookmarkBarView:
  bool UpdateOtherAndManagedButtonsVisibility() override;

 private:
  // Note that so-called "Others button" is renamed to "All bookmarks button"
  void OnShowAllBookmarksButtonPrefChanged();

  void MaybeUpdateOtherAndManagedButtonsVisibility();

  BooleanPrefMember show_all_bookmarks_button_pref_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BOOKMARKS_BRAVE_BOOKMARK_BAR_VIEW_H_
