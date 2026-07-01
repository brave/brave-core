// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_LAYOUT_CONSTANTS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_LAYOUT_CONSTANTS_H_

// Use our own version of GetLayoutConstant to include Brave values.
#define GetLayoutConstant GetLayoutConstant_ChromiumImpl
#include <chrome/browser/ui/layout_constants.h>  // IWYU pragma: export
#undef GetLayoutConstant

int GetLayoutConstant(LayoutConstant constant);

// Rounded corners.
// The padding between the main content area and the window edges (and other
// surrounding chrome such as vertical tabs and the sidebar control).
inline constexpr int kRoundedCornersContentsViewMargin = 4;

// The gap between the main content area and the side panel when both are shown
// with rounded corners. Kept separate from kRoundedCornersContentsViewMargin so
// the content<->panel spacing can be tuned independently of the content<->edge
// padding.
inline constexpr int kRoundedCornersContentsSidePanelGap = 5;

namespace tabs {

// Horizontal tab layout:
//
// The upstream tab implementation assumes that tab view bounds overlap. In
// order to create a gap between tabs without violating these assumptions, tabs
// views are given a small overlap. Rounded tab rectangles are drawn centered
// and inset horizontally by an amount that will create the required visual gap.

// The visual height of tabs in horizontal tabs mode. Note that the height of
// the view may be greater than the visual height of the tab shape.
int GetHorizontalTabHeight();

// Returns the amount of visual spacing between the top and bottom of tabs and
// the bounds of the tab strip region. The portion of this space below tabs will
// be occupied by tab group underlines.
int GetHorizontalTabVerticalSpacing();

// The amount of space before the first tab view.
inline constexpr int kHorizontalTabStripLeftMargin = 3;

// The amount of visual spacing between the top and bottom of tabs and the
// bounds of the tab strip region for a split view tab.
inline constexpr int kHorizontalSplitViewTabVerticalSpacing = 6;

// The height of the tab strip in horizontal mode.
int GetHorizontalTabStripHeight();

// The visual gap between tabs.
inline constexpr int kHorizontalTabGap = 4;

// The amount of overlap between tabs. Based on upstream assumptions, tab views
// must have a non-negative overlap. Furthermore, tab separators will not render
// correctly if the tab overlap is zero.
inline constexpr int kHorizontalTabOverlap = 2;

// The horizontal difference between the edge of the tab view and the visual
// edge of the rendered tab.
inline constexpr int kHorizontalTabInset =
    (kHorizontalTabGap + kHorizontalTabOverlap) / 2;

// The horizontal content padding within a tab.
int GetHorizontalTabPadding();

// The horizontal difference between the visual edge of a tab group and the
// bounds of the group underline.
inline constexpr int kHorizontalGroupUnderlineInset = 2;

// The tab border radius.
inline constexpr int kTabBorderRadius = 8;

// The line height of the tab group header text.
inline constexpr int kTabGroupLineHeight = 24;

// The amount of padding at the top and bottom of tab group header "chips"
// (i.e. the rectangle enclosing the tab group title).
int GetTabGroupTitleVerticalInset();

// The amount of padding at sides of tab group header "chips".
int GetTabGroupTitleHorizontalInset();

// The amount of space at the top of inactive tabs where mouse clicks are
// treated as clicks in the "caption" area, i.e. the draggable part of the
// window frame.
int GetDragHandleExtensionHeight();

// Indicates whether horizontal tabs (and the toolbar) are displayed in compact
// mode.
bool UseCompactHorizontalTabs();

}  // namespace tabs

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_LAYOUT_CONSTANTS_H_
