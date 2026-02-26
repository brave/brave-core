/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/bookmarks/brave_bookmark_bar_view.h"

#include "brave/browser/ui/bookmark/brave_bookmark_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/bookmarks/bookmark_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/controls/button/label_button.h"

BraveBookmarkBarView::BraveBookmarkBarView(Browser* browser,
                                           BrowserView* browser_view)
    : BookmarkBarView(browser, browser_view) {
  show_all_bookmarks_button_pref_.Init(
      brave::bookmarks::prefs::kShowAllBookmarksButton,
      browser_->profile()->GetPrefs(),
      base::BindRepeating(
          &BraveBookmarkBarView::OnShowAllBookmarksButtonPrefChanged,
          base::Unretained(this)));

  MaybeUpdateOtherAndManagedButtonsVisibility();
}

BraveBookmarkBarView::~BraveBookmarkBarView() = default;

void BraveBookmarkBarView::ConfigureButton(
    const bookmarks::BookmarkNode* node,
    views::LabelButton* button) {
  BookmarkBarView::ConfigureButton(node, button);
  if (node->is_folder()) {
    button->SetImageModel(
        views::Button::STATE_NORMAL,
        chrome::GetBookmarkFolderIcon(chrome::BookmarkFolderIconType::kNormal,
                                      kColorToolbarButtonIcon));
    button->SetImageModel(
        views::Button::STATE_DISABLED,
        chrome::GetBookmarkFolderIcon(chrome::BookmarkFolderIconType::kNormal,
                                      kColorToolbarButtonIconInactive));
  }
}

void BraveBookmarkBarView::UpdateAppearanceForTheme() {
  BookmarkBarView::UpdateAppearanceForTheme();

  if (all_bookmarks_button_) {
    all_bookmarks_button_->SetImageModel(
        views::Button::STATE_NORMAL,
        chrome::GetBookmarkFolderIcon(chrome::BookmarkFolderIconType::kNormal,
                                      kColorToolbarButtonIcon));
    all_bookmarks_button_->SetImageModel(
        views::Button::STATE_DISABLED,
        chrome::GetBookmarkFolderIcon(chrome::BookmarkFolderIconType::kNormal,
                                      kColorToolbarButtonIconInactive));
  }

  if (managed_bookmarks_button_) {
    managed_bookmarks_button_->SetImageModel(
        views::Button::STATE_NORMAL,
        chrome::GetBookmarkFolderIcon(chrome::BookmarkFolderIconType::kManaged,
                                      kColorToolbarButtonIcon));
    managed_bookmarks_button_->SetImageModel(
        views::Button::STATE_DISABLED,
        chrome::GetBookmarkFolderIcon(chrome::BookmarkFolderIconType::kManaged,
                                      kColorToolbarButtonIconInactive));
  }
}

void BraveBookmarkBarView::MaybeUpdateOtherAndManagedButtonsVisibility() {
  if (bookmark_service_ && bookmark_service_->bookmark_model() &&
      bookmark_service_->bookmark_model()->loaded()) {
    UpdateOtherAndManagedButtonsVisibility();
  }
}

bool BraveBookmarkBarView::UpdateOtherAndManagedButtonsVisibility() {
  bool result = BookmarkBarView::UpdateOtherAndManagedButtonsVisibility();
  if (all_bookmarks_button_ && all_bookmarks_button_->GetVisible() &&
      !show_all_bookmarks_button_pref_.GetValue()) {
    all_bookmarks_button_->SetVisible(false);
    UpdateBookmarksSeparatorVisibility();
    return true;
  }
  return result;
}

void BraveBookmarkBarView::OnShowAllBookmarksButtonPrefChanged() {
  if (UpdateOtherAndManagedButtonsVisibility()) {
    UpdateBookmarksSeparatorVisibility();
    LayoutAndPaint();
  }
}

BEGIN_METADATA(BraveBookmarkBarView)
END_METADATA
