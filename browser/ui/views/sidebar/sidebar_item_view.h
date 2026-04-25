/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_VIEW_H_

#include <string>

#include "brave/browser/ui/views/sidebar/sidebar_button_view.h"

class SidebarItemView : public SidebarButtonView {
  METADATA_HEADER(SidebarItemView, SidebarButtonView)
 public:
  explicit SidebarItemView(const std::u16string& accessible_name);
  ~SidebarItemView() override;

  SidebarItemView(const SidebarItemView&) = delete;
  SidebarItemView operator=(const SidebarItemView&) = delete;

  void SetActiveState(bool active);

  void DrawHorizontalBorder(bool top);
  void ClearHorizontalBorder();

  // SidebarButtonView overrides:
  void OnPaintBorder(gfx::Canvas* canvas) override;
  bool IsTriggerableEvent(const ui::Event& e) override;
  void StateChanged(ButtonState old_state) override;
  void OnThemeChanged() override;

 private:
  bool active_ = false;
  bool draw_horizontal_border_ = false;
  bool draw_horizontal_border_top_ = false;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_VIEW_H_
