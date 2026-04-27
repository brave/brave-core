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

namespace tabs {

// Horizontal tab layout:
//
// The upstream tab implemenation assumes that tab view bounds overlap. In order
// to create a gap between tabs without violating these assumptions, tabs views
// are given a small overlap. Rounded tab rectangles are drawn centered and
// inset horizontally by an amount that will create the required visual gap.

// The visual height of tabs in horizontal tabs mode. Note that the height of
// the view may be greater than the visual height of the tab shape. See also
// `GetHorizontalTabVerticalSpacing()`.
int GetHorizontalTabHeight();

// Spacing between the top and bottom of tabs and the tab strip bounds. Use
// this instead of `kHorizontalTabVerticalSpacing` for layout.
//
// Returns `compact_horizontal_tabs_layout::kTabVerticalSpacing` (2) when the
// `#brave-compact-horizontal-tabs` flag is enabled and touch UI is off.
// Returns `compact_horizontal_tabs_layout::kTabVerticalSpacingDefault` (which
// equals `kHorizontalTabVerticalSpacing`, 4) otherwise (flag disabled or touch
// UI active).
int GetHorizontalTabVerticalSpacing();

// Vertical delta (DIP) used by tab strip control button placement math (see
// `UpdateButtonBorders()` in horizontal_tab_strip_region_view.cc). This is
// intentionally separate from `LayoutConstant::kTabstripToolbarOverlap` so
// tab/toolbar geometry can be tuned without unintentionally shifting
// navigation/caption controls. See the comment on
// `compact_horizontal_tabs_layout::kTabStripControlsHeightDelta` for the
// rationale behind the split.
//
// Returns `compact_horizontal_tabs_layout::kTabStripControlsHeightDelta` (-5)
// when the `#brave-compact-horizontal-tabs` flag is enabled and touch UI is
// off. Returns
// `compact_horizontal_tabs_layout::kTabStripControlsHeightDeltaDefault`
// (-4) otherwise (flag disabled or touch UI active).
int GetHorizontalTabControlsDelta();

// The amount of space before the first tab view.
inline constexpr int kHorizontalTabStripLeftMargin = 3;

// Default vertical spacing when not using compact horizontal tabs (and for
// touch UI). `GetHorizontalTabVerticalSpacing()` may return a smaller value
// when
// `#brave-compact-horizontal-tabs` is enabled.
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

}  // namespace tabs

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_LAYOUT_CONSTANTS_H_
