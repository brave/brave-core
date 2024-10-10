/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_strip_layout_helper.h"

#include "brave/browser/ui/views/tabs/brave_tab_strip.h"

#define CalculateTabBounds                                                     \
  FillTiledState(tab_widths, static_cast<BraveTabStrip*>(tab_strip_.get())) && \
          use_vertical_tabs_&& FillGroupInfo(tab_widths)                       \
      ? tabs::CalculateVerticalTabBounds(                                      \
            layout_constants, tab_widths, tabstrip_width,                      \
            GetBraveTabStrip() -> IsVerticalTabsFloating())                    \
      : CalculateTabBounds

#include "src/chrome/browser/ui/views/tabs/tab_strip_layout_helper.cc"

#undef CalculateTabBounds

BraveTabStrip* TabStripLayoutHelper::GetBraveTabStrip() const {
  return static_cast<BraveTabStrip*>(tab_strip_.get());
}

// Unfortunately, TabStripLayout::TabSlot is declared and defined in the .cc
// file, we can't move this method out of this file.
bool TabStripLayoutHelper::FillGroupInfo(
    std::vector<TabWidthConstraints>& tab_widths) {
  DCHECK(use_vertical_tabs_)
      << "Must be called only when |use_vertical_tabs_| is true";

  for (auto i = 0u; i < tab_widths.size(); i++) {
    auto& tab_width_constraints = tab_widths.at(i);
    tab_width_constraints.set_is_tab_in_group(
        slots_.at(i).type == ViewType::kTab &&
        slots_.at(i).view->group().has_value());
  }
  return true;
}

bool TabStripLayoutHelper::FillTiledState(
    std::vector<TabWidthConstraints>& tab_widths,
    BraveTabStrip* tab_strip) {
  if (!tab_strip_) {
    // This method can be called before the tab strip is set.
    return true;
  }

  for (int i = 0; i < static_cast<int>(slots_.size()); i++) {
    auto index = GetBraveTabStrip()->GetModelIndexOf(slots_.at(i).view);
    if (!index) {
      continue;
    }

    auto& tab_width = tab_widths[i];
    tab_width.state().set_tiled_state(tab_strip->GetTiledStateForTab(*index));
  }
  return true;
}
