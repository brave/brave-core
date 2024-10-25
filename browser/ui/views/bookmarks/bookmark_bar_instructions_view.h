/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BOOKMARKS_BOOKMARK_BAR_INSTRUCTIONS_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BOOKMARKS_BOOKMARK_BAR_INSTRUCTIONS_VIEW_H_

#include "base/compiler_specific.h"
#include "base/memory/raw_ptr.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/view.h"

namespace views {
class Label;
class Link;
}  // namespace views

class Browser;

// BookmarkBarInstructionsView is a child of the bookmark bar that is visible
// when the user has no bookmarks on the bookmark bar.
// BookmarkBarInstructionsView shows a description of the bookmarks bar along
// with a link to import bookmarks.
// NOTE: Most of code is copied from chromium's deleted file.
class BookmarkBarInstructionsView : public views::View,
                                    public views::ContextMenuController {
  METADATA_HEADER(BookmarkBarInstructionsView, views::View)
 public:
  explicit BookmarkBarInstructionsView(Browser* browser);
  BookmarkBarInstructionsView(const BookmarkBarInstructionsView&) = delete;
  BookmarkBarInstructionsView& operator=(const BookmarkBarInstructionsView&) =
      delete;

 private:
  // views::View:
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout(PassKey) override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

  // views::ContextMenuController:
  void ShowContextMenuForViewImpl(
      views::View* source,
      const gfx::Point& point,
      ui::mojom::MenuSourceType source_type) override;

  void UpdateColors();
  void LinkClicked();
  SkColor GetInstructionsTextColor();

  raw_ptr<views::Label> instructions_ = nullptr;
  raw_ptr<views::Link> import_link_ = nullptr;

  raw_ptr<Browser> browser_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BOOKMARKS_BOOKMARK_BAR_INSTRUCTIONS_VIEW_H_
