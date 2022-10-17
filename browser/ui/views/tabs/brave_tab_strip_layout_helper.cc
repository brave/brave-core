/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_strip_layout_helper.h"

#include "brave/browser/ui/views/tabs/brave_tab_group_header.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/tabs/tab_types.h"
#include "chrome/browser/ui/views/tabs/tab_strip_layout.h"
#include "chrome/browser/ui/views/tabs/tab_width_constraints.h"
#include "ui/gfx/geometry/rect.h"

namespace tabs {

std::vector<gfx::Rect> CalculateVerticalTabBounds(
    const TabLayoutConstants& layout_constants,
    const std::vector<TabWidthConstraints>& tabs,
    absl::optional<int> width) {
  if (tabs.empty())
    return std::vector<gfx::Rect>();

  std::vector<gfx::Rect> bounds;
  gfx::Rect rect;
  for (const auto& tab : tabs) {
    rect.set_x(tab.is_tab_in_group()
                   ? BraveTabGroupHeader::GetLeftPaddingForVerticalTabs()
                   : 0);
    rect.set_width(width.value_or(tabs.front().GetPreferredWidth()) - rect.x());
    rect.set_height(
        tab.state().open() == TabOpen::kOpen ? layout_constants.tab_height : 0);
    bounds.push_back(rect);
    if (tab.state().open() == TabOpen::kOpen)
      rect.set_y(rect.bottom());
  }
  return bounds;
}

}  // namespace tabs
