/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_VIEW_H_

#include <string>

#include "brave/browser/ui/views/sidebar/sidebar_button_view.h"

class SidebarItemView : public SidebarButtonView {
 public:
  METADATA_HEADER(SidebarItemView);
  explicit SidebarItemView(const std::u16string& accessible_name);
  ~SidebarItemView() override;

  SidebarItemView(const SidebarItemView&) = delete;
  SidebarItemView operator=(const SidebarItemView&) = delete;

  void set_draw_highlight(bool draw) { draw_highlight_ = draw; }
  void set_draw_highlight_on_left(bool draw_on_left) {
    draw_highlight_on_left_ = draw_on_left;
  }

  void set_paint_background_on_hovered(bool paint) {
    paint_background_on_hovered_ = paint;
  }

  void DrawHorizontalBorder(bool top);
  void ClearHorizontalBorder();

  // views::ImageButton overrides:
  void OnPaintBorder(gfx::Canvas* canvas) override;
  bool IsTriggerableEvent(const ui::Event& e) override;

 private:
  bool draw_highlight_ = false;
  bool draw_highlight_on_left_ = false;
  bool paint_background_on_hovered_ = false;
  bool draw_horizontal_border_ = false;
  bool draw_horizontal_border_top_ = false;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_VIEW_H_
