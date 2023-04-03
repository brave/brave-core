/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_LAYOUT_MANAGER_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_LAYOUT_MANAGER_H_

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/views/frame/contents_layout_manager.h"

class BraveContentsLayoutManager : public ContentsLayoutManager {
 public:
  BraveContentsLayoutManager(views::View* devtools_view,
                             views::View* contents_view,
                             views::View* sidebar_container_view);
  BraveContentsLayoutManager(const BraveContentsLayoutManager&) = delete;
  BraveContentsLayoutManager& operator=(const BraveContentsLayoutManager&) =
      delete;
  ~BraveContentsLayoutManager() override;

  void set_sidebar_on_left(bool sidebar_on_left) {
    sidebar_on_left_ = sidebar_on_left;
  }

  void set_reader_mode_panel(views::View* reader_mode_panel_view) {
    reader_mode_panel_view_ = reader_mode_panel_view;
  }

  // ContentsLayoutManager overrides:
  void Layout(views::View* contents_container) override;

 private:
  raw_ptr<views::View> sidebar_container_view_ = nullptr;
  raw_ptr<views::View> reader_mode_panel_view_ = nullptr;
  bool sidebar_on_left_ = true;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_LAYOUT_MANAGER_H_
