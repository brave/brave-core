/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_LAYOUT_MANAGER_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_LAYOUT_MANAGER_H_

#include "chrome/browser/ui/views/frame/contents_layout_manager.h"

class BraveContentsLayoutManager : public ContentsLayoutManager {
 public:
  static BraveContentsLayoutManager* GetLayoutManagerForView(views::View* host);

  BraveContentsLayoutManager(views::View* devtools_view,
                             views::View* contents_view,
                             views::View* lens_overlay_view,
                             views::View* scrim_view,
                             views::View* border_view,
                             views::View* watermark_view,
                             views::View* reader_mode_toolbar);
  ~BraveContentsLayoutManager() override;

  // We want to set a contents container border for split view.
  // However, upstream ContentsLayoutManager doesn't consider host view's
  // border when layout web contents. So, adjust its border when calculating
  // proposed layout with |border_insets_|.
  void SetWebContentsBorderInsets(const gfx::Insets& insets);

 protected:
  // ContentsLayoutManager:
  views::ProposedLayout CalculateProposedLayout(
      const views::SizeBounds& size_bounds) const override;

 private:
  raw_ptr<views::View> contents_view_ = nullptr;
  raw_ptr<views::View> reader_mode_toolbar_ = nullptr;
  gfx::Insets border_insets_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_LAYOUT_MANAGER_H_
