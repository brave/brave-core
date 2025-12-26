/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_MINI_TOOLBAR_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_MINI_TOOLBAR_H_

#include "brave/browser/ui/color/brave_color_id.h"
#include "chrome/browser/ui/views/frame/multi_contents_view_mini_toolbar.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BraveMultiContentsViewMiniToolbar : public MultiContentsViewMiniToolbar {
  METADATA_HEADER(BraveMultiContentsViewMiniToolbar,
                  MultiContentsViewMiniToolbar)
 public:
  using MultiContentsViewMiniToolbar::MultiContentsViewMiniToolbar;
  ~BraveMultiContentsViewMiniToolbar() override;

  static BraveMultiContentsViewMiniToolbar* From(
      MultiContentsViewMiniToolbar* toolbar);

  // View:
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override;

  // MultiContentsViewMiniToolbar:
  void UpdateState(bool is_active, bool is_highlighted) override;
  void OnPaint(gfx::Canvas* canvas) override;

  void HideMenuButton();

 private:
  int GetOutlineThickness() const;
  SkPath GetPath(bool border_stroke_only) const;

  bool is_active_ = false;
  ui::ColorId stroke_color_ = kColorBraveSplitViewInactiveWebViewBorder;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_MINI_TOOLBAR_H_
