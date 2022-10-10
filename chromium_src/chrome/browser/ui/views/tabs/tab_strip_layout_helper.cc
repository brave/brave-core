/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_strip_layout_helper.h"

#define CalculateTabBounds                                             \
  use_vertical_tabs_&& tabs::FillGroupInfo(tab_widths, *this)          \
      ? tabs::CalculateVerticalTabBounds(layout_constants, tab_widths, \
                                         tabstrip_width)               \
      : CalculateTabBounds

#include "src/chrome/browser/ui/views/tabs/tab_strip_layout_helper.cc"

#undef CalculateTabBounds

namespace tabs {

// Defines this function intentionally after we include
// tab_strip_layout_helper.cc. This function depends on inner class of
// TabStripLayoutHelper, so we need to be accessible to the class(TabSlot).
bool FillGroupInfo(std::vector<TabWidthConstraints>& tab_widths,
                   TabStripLayoutHelper& helper) {
  for (auto i = 0u; i < tab_widths.size(); i++) {
    auto& tab_width_constraints = tab_widths.at(i);
    tab_width_constraints.set_is_tab_in_group(
        helper.slots_.at(i).type == ViewType::kTab &&
        helper.slots_.at(i).view->group().has_value());
  }
  return true;
}

}  // namespace tabs
