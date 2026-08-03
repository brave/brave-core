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

  // Keeps the favicon and domain visible while this toolbar's contents area is
  // active. By default they are shown only while it is inactive. Takes effect
  // on the next `UpdateState()` call.
  void SetAlwaysShowDomain(bool always_show_domain);

  // Sets whether this toolbar meets a split view border, as opposed to the
  // thinner contents outline drawn around a contents area that stands alone.
  // Defaults to true. Takes effect on the next `UpdateState()` call.
  void SetUsesSplitBorder(bool uses_split_border);

  // View:
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override;

  // MultiContentsViewMiniToolbar:
  void UpdateContents() override;
  void UpdateState(bool is_active, bool is_highlighted) override;
  void OnPaint(gfx::Canvas* canvas) override;

  // Permanently removes the menu button from this toolbar.
  void HideMenuButton();

 private:
  int GetOutlineThickness() const;
  int GetContainerBorderThickness() const;
  SkPath GetPath(bool border_stroke_only) const;
  void UpdateClipPath();

  bool is_active_ = false;
  bool always_show_domain_ = false;
  bool uses_split_border_ = true;
  bool has_menu_button_ = true;
  ui::ColorId stroke_color_ = kColorBraveSplitViewInactiveWebViewBorder;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_MINI_TOOLBAR_H_
