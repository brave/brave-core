
/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BOOKMARKS_BRAVE_BOOKMARK_BAR_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BOOKMARKS_BRAVE_BOOKMARK_BAR_VIEW_H_

#include "base/callback_list.h"
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

  // views::View:
  void AddedToWidget() override;
  void RemovedFromWidget() override;
  void OnThemeChanged() override;

 private:
  // Note that so-called "Others button" is renamed to "All bookmarks button"
  void OnShowAllBookmarksButtonPrefChanged();

  void MaybeUpdateOtherAndManagedButtonsVisibility();
  void UpdateFolderIconsForActiveState();

  BooleanPrefMember show_all_bookmarks_button_pref_;
  base::CallbackListSubscription paint_as_active_subscription_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BOOKMARKS_BRAVE_BOOKMARK_BAR_VIEW_H_
