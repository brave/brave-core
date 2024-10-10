/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_LAYOUT_MANAGER_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_LAYOUT_MANAGER_H_

#include "chrome/browser/ui/views/frame/contents_layout_manager.h"

class BraveBrowserView;

class BraveContentsLayoutManager : public ContentsLayoutManager {
 public:
  // Spacing between |contents_web_view_| and |secondary_contents_web_view_|.
  static constexpr auto kSpacingBetweenContentsWebViews = 4;

  BraveContentsLayoutManager(views::View* devtools_view,
                             views::View* contents_view,
                             views::View* watermark_view = nullptr);
  ~BraveContentsLayoutManager() override;

  void set_browser_view(BraveBrowserView* browser_view) {
    browser_view_ = browser_view;
  }

  void set_contents_reader_mode_toolbar(
      views::View* contents_reader_mode_toolbar) {
    contents_reader_mode_toolbar_ = contents_reader_mode_toolbar;
  }

 protected:
  // ContentsLayoutManager:
  void LayoutImpl() override;

  void LayoutContents(const gfx::Rect& bounds,
                      views::View* contents_view,
                      views::View* reader_mode_toolbar,
                      views::View* devtools_view,
                      const DevToolsContentsResizingStrategy& strategy);

 private:
  friend class BraveContentsLayoutManagerUnitTest;
  friend class SplitViewContentsLayoutManager;

  raw_ptr<BraveBrowserView> browser_view_ = nullptr;

  raw_ptr<views::View> contents_reader_mode_toolbar_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_LAYOUT_MANAGER_H_
