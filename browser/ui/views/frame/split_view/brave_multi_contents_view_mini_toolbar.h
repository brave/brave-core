/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_MINI_TOOLBAR_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_MINI_TOOLBAR_H_

#include "chrome/browser/ui/views/frame/multi_contents_view_mini_toolbar.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BraveMultiContentsViewMiniToolbar : public MultiContentsViewMiniToolbar {
  METADATA_HEADER(BraveMultiContentsViewMiniToolbar,
                  MultiContentsViewMiniToolbar)
 public:
  using MultiContentsViewMiniToolbar::MultiContentsViewMiniToolbar;
  ~BraveMultiContentsViewMiniToolbar() override;

  // MultiContentsViewMiniToolbar:
  void UpdateState(bool is_active) override;
  void OnPaint(gfx::Canvas* canvas) override;
  SkPath GetPath(bool border_stroke_only) const override;

 private:
  int GetOutlineThickness() const;

  bool is_active_ = false;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_MINI_TOOLBAR_H_
