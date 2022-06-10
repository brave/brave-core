/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_strip_layout.h"

#include "chrome/browser/ui/tabs/tab_style.h"
#include "ui/gfx/geometry/rect.h"

std::vector<gfx::Rect> CalculateVerticalTabBounds(
    const TabLayoutConstants& layout_constants,
    const std::vector<TabWidthConstraints>& tabs,
    absl::optional<int> width) {
  if (tabs.empty())
    return std::vector<gfx::Rect>();

  std::vector<gfx::Rect> bounds;
  gfx::Rect rect;
  rect.set_width(width.value_or(tabs.front().GetPreferredWidth()));
  rect.set_height(layout_constants.tab_height);
  for (const auto& tab : tabs) {
    bounds.push_back(rect);
    // Workaround to check if a tab is closing now. There's a tight coupling
    // with TabWidthConstraints::TransformForPinnednessAndOpeneness()'s
    // implementation.
    const bool is_opened = tab.GetPreferredWidth() != TabStyle::GetTabOverlap();
    if (is_opened)
      rect.set_y(rect.bottom());
  }
  return bounds;
}
