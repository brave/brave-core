/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_LAYOUT_HELPER_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_LAYOUT_HELPER_H_

#include <vector>

#include "brave/browser/ui/views/sidebar/sidebar_item_view.h"
#include "components/tab_groups/tab_group_id.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace gfx {
class Rect;
}  // namespace gfx

class TabStripLayoutHelper;
class TabWidthConstraints;
class TabStripController;
class TabContainer;
class TabSlotView;
class TabStrip;
struct TabLayoutConstants;

namespace tabs {

constexpr int kVerticalTabHeight = 36;
constexpr int kVerticalTabMinWidth = kVerticalTabHeight;
constexpr int kVerticalTabsSpacing = 4;
constexpr int kMarginForVerticalTabContainers = kVerticalTabsSpacing;

constexpr int kUnpinnedTabBorderRadius = 4;
constexpr int kPinnedTabBorderRadius = 8;

std::vector<gfx::Rect> CalculateVerticalTabBounds(
    const TabLayoutConstants& layout_constants,
    const std::vector<TabWidthConstraints>& tabs,
    absl::optional<int> width,
    bool is_floating_mode);

std::vector<gfx::Rect> CalculateBoundsForVerticalDraggedViews(
    const std::vector<TabSlotView*>& views,
    TabStrip* tab_strip);

void UpdateInsertionIndexForVerticalTabs(
    const gfx::Rect& dragged_bounds,
    int first_dragged_tab_index,
    int num_dragged_tabs,
    absl::optional<tab_groups::TabGroupId> dragged_group,
    int candidate_index,
    TabStripController* tab_strip_controller,
    TabContainer* tab_container,
    int& min_distance,
    int& min_distance_index,
    TabStrip* tab_strip);

}  // namespace tabs

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_LAYOUT_HELPER_H_
