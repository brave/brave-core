/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_strip_layout_helper.h"

#include <limits>
#include <optional>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
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
    const std::vector<TabWidthConstraints>& tabs,
    std::optional<int> width,
    bool is_floating_mode,
    std::vector<gfx::Rect>* result) {
  DCHECK(tabs.size());
  DCHECK(result);

  // This method only cares about pinned tab container's layout.
  if (tabs[0].state().pinned() != TabPinned::kPinned) {
    return;
  }

  if (is_floating_mode) {
    // In floating mode, we should lay out pinned tabs vertically so that tabs
    // underneath the mouse cursor wouldn't move.
    return;
  }

  // Passed |true| as |is_split| but it doesn't have any meaning becuase we
  // always use same width.
  const auto available_width =
      width.value_or(TabStyle::Get()->GetStandardWidth(/*is_split*/ true));
  gfx::Rect rect(/* x= */ kMarginForVerticalTabContainers,
                 /* y= */ kMarginForVerticalTabContainers,
                 /* width= */ kVerticalTabMinWidth,
                 /* height= */ kVerticalTabHeight);
  const auto tab_count = tabs.size();
  for (size_t i = 0; i < tab_count; i++) {
    auto tab = tabs[i];
    if (tab.state().open() != TabOpen::kOpen) {
      continue;
    }

    // Don't need to consider any conditions for first tab.
    if (i == 0) {
      result->push_back(rect);
      continue;
    }

    // Check |tab| is left split tab.
    const bool need_split_tabs_check =
        (i != (tab_count - 1) && tab.state().split().has_value() &&
         (tab.state().split() == tabs[i + 1].state().split()));
    if (need_split_tabs_check) {
      // If two split tabs can't be put in the same line, move it to next line.
      // This sets left split tab. Right split tab will be handled like other
      // non split tab.
      if (rect.right() + (kVerticalTabMinWidth + kVerticalTabsSpacing) * 2 >=
          available_width) {
        // New line
        rect.set_x(kMarginForVerticalTabContainers);
        rect.set_y(rect.bottom() + kVerticalTabsSpacing);
        result->push_back(rect);
        continue;
      }
    }

    // Update rect for the next pinned tabs. If overflowed, break into new line.
    if (rect.right() + kVerticalTabMinWidth + kVerticalTabsSpacing <
        available_width) {
      rect.set_x(rect.right() + kVerticalTabsSpacing);
    } else {
      // New line
      rect.set_x(kMarginForVerticalTabContainers);
      rect.set_y(rect.bottom() + kVerticalTabsSpacing);
    }
    result->push_back(rect);
  }
}

void CalculateVerticalLayout(const std::vector<TabWidthConstraints>& tabs,
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

std::pair<std::vector<gfx::Rect>, LayoutDomain> CalculateVerticalTabBounds(
    const std::vector<TabWidthConstraints>& tabs,
    std::optional<int> width,
    bool is_floating_mode) {
  // We can return LayoutDomain::kInactiveWidthEqualsActiveWidth always because
  // vertical tab uses same width for active and inactive tabs.
  if (tabs.empty()) {
    return {std::vector<gfx::Rect>(),
            LayoutDomain::kInactiveWidthEqualsActiveWidth};
  }

  std::vector<gfx::Rect> bounds;
  CalculatePinnedTabsBoundsInGrid(tabs, width, is_floating_mode, &bounds);
  CalculateVerticalLayout(tabs, width, &bounds);

  DCHECK_EQ(tabs.size(), bounds.size());
  return {bounds, LayoutDomain::kInactiveWidthEqualsActiveWidth};
}

std::vector<gfx::Rect> CalculateBoundsForVerticalDraggedViews(
    const std::vector<TabSlotView*>& views,
    TabStrip* tab_strip) {
  const bool is_vertical_tabs_floating =
      static_cast<BraveTabStrip*>(tab_strip)->IsVerticalTabsFloating();

  std::vector<gfx::Rect> bounds;
  int x = 0;
  int y = 0;
  for (const TabSlotView* view : views) {
    auto width = tab_strip->GetDragContext()->GetTabDragAreaWidth();
    const int height = view->height();
    const bool is_slot_tab =
        view->GetTabSlotViewType() == TabSlotView::ViewType::kTab;
    if (is_slot_tab) {
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

    // unpinned dragged tabs are laid out vertically.
    y += height + kVerticalTabsSpacing;
  }
  return bounds;
}

void UpdateInsertionIndexForVerticalTabs(
    const gfx::Rect& dragged_bounds,
    int first_dragged_tab_index,
    int num_dragged_tabs,
    bool dragged_group,
    int candidate_index,
    TabStripController* tab_strip_controller,
    TabContainer* tab_container,
    int& min_distance,
    int& min_distance_index,
    TabStrip* tab_strip) {
  // We don't allow tab groups to be dragged over pinned tabs area.
  if (dragged_group && candidate_index != 0 &&
      tab_strip_controller->IsTabPinned(candidate_index - 1)) {
    return;
  }

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
