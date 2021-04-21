/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_VIEW_H_

#include "brave/browser/ui/views/sidebar/sidebar_button_view.h"

class SidebarItemView : public SidebarButtonView {
 public:
  explicit SidebarItemView(Delegate* delegate);
  ~SidebarItemView() override;

  SidebarItemView(const SidebarItemView&) = delete;
  SidebarItemView operator=(const SidebarItemView&) = delete;

  void set_draw_highlight(bool draw) { draw_highlight_ = draw; }

  void set_paint_background_on_hovered(bool paint) {
    paint_background_on_hovered_ = paint;
  }

  void DrawHorizontalBorder(bool top);
  void ClearHorizontalBorder();

  // views::ImageButton overrides:
  void OnPaintBackground(gfx::Canvas* canvas) override;
  void OnPaintBorder(gfx::Canvas* canvas) override;

 private:
  bool draw_highlight_ = false;
  bool paint_background_on_hovered_ = false;
  bool draw_horizontal_border_ = false;
  bool draw_horizontal_border_top_ = false;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_VIEW_H_
