/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_strip_layout_helper.h"

#include <limits>
#include <optional>

#include "brave/browser/ui/tabs/brave_tab_layout_constants.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/tabs/brave_tab_group_header.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/tabs/tab_types.h"
#include "chrome/browser/ui/views/tabs/tab_container.h"
#include "chrome/browser/ui/views/tabs/tab_strip_controller.h"
#include "chrome/browser/ui/views/tabs/tab_strip_layout.h"
#include "chrome/browser/ui/views/tabs/tab_width_constraints.h"
#include "ui/gfx/geometry/rect.h"

namespace tabs {

namespace {

void CalculatePinnedTabsBoundsInGrid(
    const TabLayoutConstants& layout_constants,
    const std::vector<TabWidthConstraints>& tabs,
    std::optional<int> width,
    bool is_floating_mode,
    std::vector<gfx::Rect>* result) {
  DCHECK(tabs.size());
  DCHECK(result);

  if (is_floating_mode) {
    // In floating mode, we should lay out pinned tabs vertically so that tabs
    // underneath the mouse cursor wouldn't move.
    return;
  }

  auto* tab_style = TabStyle::Get();

  gfx::Rect rect(/* x= */ kMarginForVerticalTabContainers,
                 /* y= */ kMarginForVerticalTabContainers,
                 /* width= */ kVerticalTabMinWidth,
                 /* height= */ kVerticalTabHeight);
  for (const auto& tab : tabs) {
    if (tab.state().pinned() != TabPinned::kPinned) {
      break;
    }

    result->push_back(rect);

    if (tab.state().open() != TabOpen::kOpen) {
      continue;
    }

    // Update rect for the next pinned tabs. If overflowed, break into new line
    if (rect.right() + kVerticalTabMinWidth + kVerticalTabsSpacing <
        width.value_or(tab_style->GetStandardWidth())) {
      rect.set_x(rect.right() + kVerticalTabsSpacing);
    } else {
      // New line
      rect.set_x(kMarginForVerticalTabContainers);
      rect.set_y(result->back().bottom() + kVerticalTabsSpacing);
    }
  }
}

void CalculateVerticalLayout(const TabLayoutConstants& layout_constants,
                             const std::vector<TabWidthConstraints>& tabs,
                             std::optional<int> width,
                             std::vector<gfx::Rect>* result) {
  DCHECK(tabs.size());
  DCHECK(result);

  if (result->size() == tabs.size()) {
    // No tabs to lay out vertically.
    return;
  }

  if (!result->empty()) {
    // Usually this shouldn't happen, as pinned tabs and unpinned tabs belong to
    // separated containers. But this could happen on start-up. In this case,
    // fills bounds for unpinned tabs with empty rects.
    while (result->size() < tabs.size()) {
      result->push_back({});
    }
    return;
  }

  gfx::Rect rect;
  rect.set_y(kMarginForVerticalTabContainers);
  for (auto iter = tabs.begin() + result->size(); iter != tabs.end(); iter++) {
    auto& tab = *iter;
    rect.set_x(
        kMarginForVerticalTabContainers +
        (tab.is_tab_in_group() ? BraveTabGroupHeader::kPaddingForGroup : 0));
    rect.set_width(width.value_or(tab.GetPreferredWidth()) - rect.x() * 2);
    rect.set_height(tab.state().open() == TabOpen::kOpen ? kVerticalTabHeight
                                                         : 0);
    result->push_back(rect);

    // Update rect for the next tab.
    if (tab.state().open() == TabOpen::kOpen) {
      rect.set_y(rect.bottom() + kVerticalTabsSpacing);
    }
  }
}

}  // namespace

int GetTabCornerRadius(const Tab& tab) {
  if (!tabs::features::HorizontalTabsUpdateEnabled()) {
    return tab.data().pinned ? 8 : 4;
  }

  return brave_tabs::kTabBorderRadius;
}

std::vector<gfx::Rect> CalculateVerticalTabBounds(
    const TabLayoutConstants& layout_constants,
    const std::vector<TabWidthConstraints>& tabs,
    std::optional<int> width,
    bool is_floating_mode) {
  if (tabs.empty()) {
    return std::vector<gfx::Rect>();
  }

  std::vector<gfx::Rect> bounds;
  CalculatePinnedTabsBoundsInGrid(layout_constants, tabs, width,
                                  is_floating_mode, &bounds);
  CalculateVerticalLayout(layout_constants, tabs, width, &bounds);

  DCHECK_EQ(tabs.size(), bounds.size());
  return bounds;
}

std::vector<gfx::Rect> CalculateBoundsForVerticalDraggedViews(
    const std::vector<raw_ptr<TabSlotView, VectorExperimental>>& views,
    TabStrip* tab_strip) {
  const bool is_vertical_tabs_floating =
      static_cast<BraveTabStrip*>(tab_strip)->IsVerticalTabsFloating();

  std::vector<gfx::Rect> bounds;
  int x = 0;
  int y = 0;
  for (const TabSlotView* view : views) {
    auto width = tab_strip->GetDragContext()->GetTabDragAreaWidth();
    const int height = view->height();
    if (view->GetTabSlotViewType() == TabSlotView::ViewType::kTab) {
      if (!is_vertical_tabs_floating &&
          static_cast<const Tab*>(view)->data().pinned) {
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
    y += height + kVerticalTabsSpacing;
  }
  return bounds;
}

void UpdateInsertionIndexForVerticalTabs(
    const gfx::Rect& dragged_bounds,
    int first_dragged_tab_index,
    int num_dragged_tabs,
    std::optional<tab_groups::TabGroupId> dragged_group,
    int candidate_index,
    TabStripController* tab_strip_controller,
    TabContainer* tab_container,
    int& min_distance,
    int& min_distance_index,
    TabStrip* tab_strip) {
  // We don't allow tab groups to be dragged over pinned tabs area.
  if (dragged_group.has_value() && candidate_index != 0 &&
      tab_strip_controller->IsTabPinned(candidate_index - 1))
    return;

  const bool is_vertical_tabs_floating =
      static_cast<BraveTabStrip*>(tab_strip)->IsVerticalTabsFloating();

  int distance = std::numeric_limits<int>::max();
  const gfx::Rect candidate_bounds =
      candidate_index == 0 ? gfx::Rect()
                           : tab_container->GetIdealBounds(candidate_index - 1);
  if (!is_vertical_tabs_floating &&
      tab_strip_controller->IsTabPinned(first_dragged_tab_index)) {
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
