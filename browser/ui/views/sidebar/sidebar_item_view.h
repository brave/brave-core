/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_VIEW_H_

#include "brave/browser/ui/views/sidebar/sidebar_button_view.h"

class SidebarItemController;

class SidebarItemView : public SidebarButtonView {
 public:
  SidebarItemView(Delegate* delegate, SidebarItemController* controller);
  ~SidebarItemView() override;

  SidebarItemView(const SidebarItemView&) = delete;
  SidebarItemView operator=(const SidebarItemView&) = delete;

  void set_draw_highlight(bool draw) { draw_highlight_ = draw; }

  void set_paint_background_on_hovered(bool paint) {
    paint_background_on_hovered_ = paint;
  }

  // views::ImageButton overrides:
  void OnPaintBackground(gfx::Canvas* canvas) override;
  void OnPaintBorder(gfx::Canvas* canvas) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnMouseCaptureLost() override;

 private:
  bool draw_highlight_ = false;
  bool paint_background_on_hovered_ = false;
  SidebarItemController* controller_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_VIEW_H_
