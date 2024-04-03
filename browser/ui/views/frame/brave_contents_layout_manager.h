/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_LAYOUT_MANAGER_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_LAYOUT_MANAGER_H_

#include "chrome/browser/ui/views/frame/contents_layout_manager.h"

class SplitViewBrowserData;

class BraveContentsLayoutManager : public ContentsLayoutManager {
 public:
  using ContentsLayoutManager::ContentsLayoutManager;
  ~BraveContentsLayoutManager() override;

  void set_secondary_contents_view(views::View* secondary_contents_view) {
    secondary_contents_view_ = secondary_contents_view;
  }

  void set_secondary_devtools_view(views::View* secondary_devtools_view) {
    secondary_devtools_view_ = secondary_devtools_view;
  }

  void set_split_view_browser_data(
      SplitViewBrowserData* split_view_browser_data) {
    split_view_browser_data_ = split_view_browser_data;
  }

  // When tile's second tab is the active web contents, we need to show the
  // tab after the first tab.
  void show_main_web_contents_at_tail(bool tail) {
    show_main_web_contents_at_tail_ = tail;
  }

 protected:
  // ContentsLayoutManager:
  void LayoutImpl() override;

 private:
  friend class BraveContentsLayoutManagerUnitTest;

  raw_ptr<SplitViewBrowserData> split_view_browser_data_ = nullptr;
  raw_ptr<views::View> secondary_contents_view_ = nullptr;
  raw_ptr<views::View> secondary_devtools_view_ = nullptr;

  bool show_main_web_contents_at_tail_ = false;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_LAYOUT_MANAGER_H_
