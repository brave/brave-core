/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_LAYOUT_MANAGER_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_LAYOUT_MANAGER_H_

#include "base/memory/raw_ptr.h"
#include "ui/views/layout/layout_manager.h"

class BraveContentsLayoutManager : public views::LayoutManager {
 public:
  BraveContentsLayoutManager(views::View* sidebar_container_view,
                             views::View* vertical_tabs_container,
                             views::View* contents_container_view);
  ~BraveContentsLayoutManager() override;

  BraveContentsLayoutManager(const BraveContentsLayoutManager&) = delete;
  BraveContentsLayoutManager operator=(const BraveContentsLayoutManager&) =
      delete;

  // views::LayoutManager overrides:
  void Layout(views::View* host) override;
  gfx::Size GetPreferredSize(const views::View* host) const override;
  void Installed(views::View* host) override;

 private:
  raw_ptr<views::View> sidebar_container_view_ = nullptr;
  raw_ptr<views::View> vertical_tabs_container_ = nullptr;
  raw_ptr<views::View> contents_container_view_ = nullptr;

  // Host is BrowserView.
  raw_ptr<views::View> host_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_LAYOUT_MANAGER_H_
