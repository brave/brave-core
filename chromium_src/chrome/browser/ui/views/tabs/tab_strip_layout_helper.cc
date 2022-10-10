/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_strip_layout_helper.h"

#define CalculateTabBounds                                             \
  use_vertical_tabs_&& FillGroupInfo(tab_widths)                       \
      ? tabs::CalculateVerticalTabBounds(layout_constants, tab_widths, \
                                         tabstrip_width)               \
      : CalculateTabBounds

#include "src/chrome/browser/ui/views/tabs/tab_strip_layout_helper.cc"

#undef CalculateTabBounds

bool TabStripLayoutHelper::FillGroupInfo(
    std::vector<TabWidthConstraints>& tab_widths) {
  for (auto i = 0u; i < tab_widths.size(); i++) {
    auto& tab_width_constraints = tab_widths.at(i);
    tab_width_constraints.set_is_tab_in_group(
        slots_.at(i).type == ViewType::kTab &&
        slots_.at(i).view->group().has_value());
  }
  return true;
}
