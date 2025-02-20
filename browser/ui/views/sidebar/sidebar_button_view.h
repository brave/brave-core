/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_BUTTON_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_BUTTON_VIEW_H_

#include <string>

#include "ui/views/controls/button/image_button.h"

class SidebarButtonView : public views::ImageButton {
  METADATA_HEADER(SidebarButtonView, views::ImageButton)
 public:
  // Value ought to follow TOOLBAR_BUTTON_HEIGHT in brave_layout_constants.cc
  static constexpr int kSidebarButtonSize = 28;
  // Value ought to follow kDefaultIconSize in toolbar_button.h
  static constexpr int kDefaultIconSize = 20;
  // External, meaning favicons for bookmarked pages
  static constexpr int kExternalIconSize = 18;
  // Vertical space between sidebar buttons
  static constexpr int kMargin = 8;

  explicit SidebarButtonView(const std::u16string& accessible_name);
  ~SidebarButtonView() override;

  SidebarButtonView(const SidebarButtonView&) = delete;
  SidebarButtonView operator=(const SidebarButtonView&) = delete;

  // views::ImageButton overrides:
  void OnThemeChanged() override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_BUTTON_VIEW_H_
