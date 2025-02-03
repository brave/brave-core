/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_LAYOUT_MANAGER_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_LAYOUT_MANAGER_H_

#include "chrome/browser/ui/views/frame/contents_layout_manager.h"

class BraveContentsLayoutManager : public ContentsLayoutManager {
 public:
  BraveContentsLayoutManager(views::View* devtools_view,
                             views::View* contents_view,
                             views::View* watermark_view,
                             views::View* reader_mode_toolbar);
  ~BraveContentsLayoutManager() override;

 protected:
  // ContentsLayoutManager:
  views::ProposedLayout CalculateProposedLayout(
      const views::SizeBounds& size_bounds) const override;

  void LayoutImpl() override;

 private:
  raw_ptr<views::View> contents_view_ = nullptr;
  raw_ptr<views::View> reader_mode_toolbar_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_LAYOUT_MANAGER_H_
