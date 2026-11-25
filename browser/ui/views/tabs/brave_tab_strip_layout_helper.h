/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_LAYOUT_HELPER_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_LAYOUT_HELPER_H_

#include <optional>
#include <utility>
#include <vector>

#include "chrome/browser/ui/views/tabs/tab_strip_layout_types.h"

namespace gfx {
class Rect;
}  // namespace gfx

namespace views {
class View;
}  // namespace views

class Tab;
class TabStripLayoutHelper;
class TabWidthConstraints;
class TabStripController;
class TabContainer;
class TabStrip;

namespace tabs {

inline constexpr int kVerticalTabHeight = 32;
inline constexpr int kVerticalTabMinWidth = kVerticalTabHeight;
inline constexpr int kVerticalTabsSpacing = 4;
inline constexpr int kMarginForVerticalTabContainers = kVerticalTabsSpacing;
inline constexpr int kPinnedUnpinnedSeparatorHeight = 1;

// The spacing between adjacent tabs in the same split in the vertical tab
// strip. The tabs' shapes are inset instead so that the gap between them can
// match the split view tile's padding.
inline constexpr int kVerticalSplitTabsSpacing = 0;

// The base offset per level for vertical tabs in tree tabs
inline constexpr int kBaseOffsetPerLevel = 20;

// Vertical gap between a tree-tab parent and its nested children so a
// dragged subtree renders as a compact, overlapping pile instead of a
// fully spaced list. Taller than kStackedOffset so the pile still reads
// as a visible stack rather than nearly-flush tabs.
inline constexpr int kNestedTabStackedOffset = 12;

// The vertical margin between the horizontal tab strip's bounds and the split
// view tile's bounding box.
inline constexpr int kHorizontalSplitViewTileVerticalMargin = 2;

// The padding between a split view tile and the tabs in it. It's also the gap
// between the tabs.
inline constexpr int kSplitViewTilePadding = 2;

int GetTabCornerRadius(const Tab& tab);

// Returns the corner radius of the tabs' shapes inside a split view tile. It's
// the same as the radius of non-split tabs.
int GetSplitViewTabCornerRadius(const Tab& tab);

// Returns the corner radius of the split view tile's bounding box. It's
// concentric with the corners of the tabs in the tile, so it's the tabs' radius
// plus the tile's padding, except for horizontal tabs without the updated
// style.
int GetSplitViewTileCornerRadius(const Tab& tab, bool is_vertical_tab);

void CalculatePinnedTabsBoundsInGrid(
    const std::vector<TabWidthConstraints>& tabs,
    std::optional<int> width,
    std::vector<gfx::Rect>* result);

std::pair<std::vector<gfx::Rect>, LayoutDomain> CalculateVerticalTabBounds(
    const std::vector<TabWidthConstraints>& tabs,
    std::optional<int> width,
    bool should_layout_pinned_tabs_in_grid);

// Core layout for a vertical-tabs drag pile, given the already-resolved drag
// area width and floating state. Exposed separately from the TabStrip*
// overload below so it can be unit tested without a live TabStrip.
std::vector<gfx::Rect> CalculateBoundsForVerticalDraggedViews(
    const std::vector<views::View*>& views,
    int drag_area_width,
    bool is_vertical_tabs_floating);

std::vector<gfx::Rect> CalculateBoundsForVerticalDraggedViews(
    const std::vector<views::View*>& views,
    TabStrip* tab_strip);

// `views` must already be children of `parent`. Reorders them so that
// `views[0]` ends up painted on top of the rest of the pile and each
// subsequent entry stacks progressively behind the previous one, matching
// the compact overlapping pile CalculateBoundsForVerticalDraggedViews
// computes bounds for. Pinned tabs are left untouched since they fan out
// horizontally instead of stacking.
void ReorderDraggedViewsForStacking(views::View* parent,
                                    const std::vector<views::View*>& views);

void UpdateInsertionIndexForVerticalTabs(
    const gfx::Rect& dragged_bounds,
    int first_dragged_tab_index,
    int num_dragged_tabs,
    bool dragged_groups,
    int candidate_index,
    TabStripController* tab_strip_controller,
    TabContainer* tab_container,
    int& min_distance,
    int& min_distance_index,
    TabStrip* tab_strip);

}  // namespace tabs

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_LAYOUT_HELPER_H_
