/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_strip_layout_helper.h"

#include <limits>

#include "brave/browser/ui/views/tabs/brave_tab_group_header.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/tabs/tab_types.h"
#include "chrome/browser/ui/views/tabs/tab_container.h"
#include "chrome/browser/ui/views/tabs/tab_strip_controller.h"
#include "chrome/browser/ui/views/tabs/tab_strip_layout.h"
#include "chrome/browser/ui/views/tabs/tab_width_constraints.h"
#include "ui/gfx/geometry/rect.h"

namespace tabs {

constexpr int kVerticalMarginForTabs = 4;

std::vector<gfx::Rect> CalculateVerticalTabBounds(
    const TabLayoutConstants& layout_constants,
    const std::vector<TabWidthConstraints>& tabs,
    absl::optional<int> width) {
  if (tabs.empty())
    return std::vector<gfx::Rect>();

  std::vector<gfx::Rect> bounds;
  gfx::Rect rect;
  bool last_tab_was_pinned = false;

  for (const auto& tab : tabs) {
    if (tab.state().pinned() == TabPinned::kPinned) {
      rect.set_width(kVerticalTabMinWidth);
      rect.set_height(kVerticalTabHeight);
      bounds.push_back(rect);

      if (tab.state().open() == TabOpen::kOpen) {
        if (rect.right() + tab.GetMinimumWidth() <
            width.value_or(TabStyle::GetStandardWidth())) {
          rect.set_x(rect.right());
        } else {
          // New line
          rect.set_x(0);
          rect.set_y(bounds.back().bottom());
        }
      }

      last_tab_was_pinned = true;
      continue;
    }

    if (last_tab_was_pinned) {
      // This could happen on start-up.
      gfx::Rect empty_rect;
      empty_rect.set_y(bounds.back().bottom());
      bounds.push_back(empty_rect);
      continue;
    }

    // This method is called by two separated containers. One contains only
    // pinned tabs and the other contains only non-pinned tabs. So we don't need
    // to consider the last pinned tab's bottom coordinate here.
    rect.set_x(tab.is_tab_in_group() ? BraveTabGroupHeader::kPaddingForGroup
                                     : 0);
    rect.set_width(width.value_or(tab.GetPreferredWidth()) - rect.x() * 2);
    rect.set_height(tab.state().open() == TabOpen::kOpen ? kVerticalTabHeight
                                                         : 0);
    bounds.push_back(rect);

    if (tab.state().open() == TabOpen::kOpen)
      rect.set_y(rect.bottom() + kVerticalMarginForTabs);
  }
  return bounds;
}

std::vector<gfx::Rect> CalculateBoundsForVerticalDraggedViews(
    const std::vector<TabSlotView*>& views) {
  std::vector<gfx::Rect> bounds;
  int x = 0;
  int y = 0;
  for (const TabSlotView* view : views) {
    auto width = TabStyle::GetStandardWidth();
    const int height = view->height();
    if (view->GetTabSlotViewType() == TabSlotView::ViewType::kTab) {
      if (const Tab* tab = static_cast<const Tab*>(view); tab->data().pinned) {
        // In case it's a pinned tab, lay out them horizontally
        bounds.emplace_back(x, y, tabs::kVerticalTabMinWidth, height);
        constexpr int kStackedOffset = 4;
        x += kStackedOffset;
        continue;
      }
      if (view->group().has_value()) {
        // In case it's a tab in a group, set left padding
        x = BraveTabGroupHeader::kPaddingForGroup;
        width -= x * 2;
      }
    }
    bounds.emplace_back(x, y, width, height);
    // unpinned dragged tabs are laid out vertically
    y += height + kVerticalMarginForTabs;
  }
  return bounds;
}

void UpdateInsertionIndexForVerticalTabs(
    const gfx::Rect& dragged_bounds,
    int first_dragged_tab_index,
    int num_dragged_tabs,
    absl::optional<tab_groups::TabGroupId> dragged_group,
    int candidate_index,
    TabStripController* tab_strip_controller,
    TabContainer* tab_container,
    int& min_distance,
    int& min_distance_index) {
  // We don't allow tab groups to be dragged over pinned tabs area.
  if (dragged_group.has_value() && candidate_index != 0 &&
      tab_strip_controller->IsTabPinned(candidate_index - 1))
    return;

  int distance = std::numeric_limits<int>::max();
  const gfx::Rect candidate_bounds =
      candidate_index == 0 ? gfx::Rect()
                           : tab_container->GetIdealBounds(candidate_index - 1);
  if (tab_strip_controller->IsTabPinned(first_dragged_tab_index)) {
    // Pinned tabs are laid out in a grid.
    distance = std::sqrt(
        std::pow(dragged_bounds.x() - candidate_bounds.CenterPoint().x(), 2) +
        std::pow(dragged_bounds.y() - candidate_bounds.CenterPoint().y(), 2));
  } else {
    // Unpinned tabs are laid out vertically. So we consider only y
    // coordinate
    distance = std::abs(dragged_bounds.y() - candidate_bounds.bottom());
  }
  if (distance < min_distance) {
    min_distance = distance;
    min_distance_index = candidate_index;
  }
}

}  // namespace tabs
