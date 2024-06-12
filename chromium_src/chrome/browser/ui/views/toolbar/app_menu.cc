/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/toolbar/app_menu.h"

#include <memory>

// Upstream uses wrong api for setting color. It comes from
// https://chromium-review.googlesource.com/c/chromium/src/+/4395705
#define SetTextColor SetTextColorId
#define SetMenuItemBackground(...)                        \
  SetMenuItemBackground(MenuItemView::MenuItemBackground( \
      background_color_id, kBackgroundCornerRadius - 6))

#define set_vertical_margin(...) set_vertical_margin(8)
#include "src/chrome/browser/ui/views/toolbar/app_menu.cc"
#undef set_vertical_margin
#undef SetMenuItemBackground
#undef SetTextColor

std::unique_ptr<views::Background>
AppMenu::CreateInMenuButtonBackgroundWithLeadingBorder() {
  return std::make_unique<InMenuButtonBackground>(
      InMenuButtonBackground::ButtonType::kLeadingBorder);
}
