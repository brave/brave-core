/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_BRAVE_TAB_LAYOUT_CONSTANTS_H_
#define BRAVE_BROWSER_UI_TABS_BRAVE_TAB_LAYOUT_CONSTANTS_H_

namespace brave_tabs {

// Horizontal tab layout:
//
// The upstream tab implemenation assumes that tab view bounds overlap. In order
// to create a gap between tabs without violating these assumptions, tabs views
// are given a small overlap. Rounded tab rectangles are drawn centered and
// inset horizontally by an amount that will create the required visual gap.

// The visual height of tabs in horizontal tabs mode. Note that the height of
// the view may be greater than the visual height of the tab shape. See also
// `kHorizontalTabVerticalSpacing`.
int GetHorizontalTabHeight();

// The amount of space before the first tab view.
inline constexpr int kHorizontalTabStripLeftMargin = 3;

// The amount of visual spacing between the top and bottom of tabs and the
// bounds of the tab strip region. The portion of this space below tabs will be
// occupied by tab group underlines.
inline constexpr int kHorizontalTabVerticalSpacing = 4;

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

// The content padding within a tab.
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

}  // namespace brave_tabs

#endif  // BRAVE_BROWSER_UI_TABS_BRAVE_TAB_LAYOUT_CONSTANTS_H_
