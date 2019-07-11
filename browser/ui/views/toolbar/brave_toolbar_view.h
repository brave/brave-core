/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_TOOLBAR_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_TOOLBAR_VIEW_H_

#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "components/prefs/pref_member.h"

class BookmarkButton;

class BraveToolbarView : public ToolbarView {
 public:
  explicit BraveToolbarView(Browser* browser, BrowserView* browser_view);
  ~BraveToolbarView() override;

  BookmarkButton* bookmark_button() const { return bookmark_; }
  void Init() override;
  void Layout() override;
  void Update(content::WebContents* tab) override;
  void OnThemeChanged() override;
  void OnEditBookmarksEnabledChanged();
  void OnLocationBarIsWideChanged();
  void ShowBookmarkBubble(const GURL& url,
                          bool already_bookmarked,
                          bookmarks::BookmarkBubbleObserver* observer) override;

 private:
  void LoadImages() override;
  void ResetLocationBarBounds();
  void ResetBookmarkButtonBounds();

  BookmarkButton* bookmark_ = nullptr;
  // Tracks the preference to determine whether bookmark editing is allowed.
  BooleanPrefMember edit_bookmarks_enabled_;
  BooleanPrefMember location_bar_is_wide_;
  // Whether this toolbar has been initialized.
  bool brave_initialized_ = false;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_TOOLBAR_VIEW_H_
