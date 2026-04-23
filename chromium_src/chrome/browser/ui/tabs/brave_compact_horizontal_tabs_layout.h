/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_BRAVE_COMPACT_HORIZONTAL_TABS_LAYOUT_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_BRAVE_COMPACT_HORIZONTAL_TABS_LAYOUT_H_

namespace tabs {
namespace compact_horizontal_tabs_layout {

// ---------------------------------------------------------------------------
// Single place to tweak `#brave-compact-horizontal-tabs` layout (DIP).
//
// Non-touch: these values are used together with
// `BraveCompactHorizontalTabsMetricsActive()` in layout_constants.cc (flag on,
// touch UI off). Goal: ~60px tab strip + toolbar row.
//
// Touch UI: only the "Tab strip (touch + compact)" values apply; toolbar and
// omnibox use the normal (non-compact) metrics so targets stay large enough.
// ---------------------------------------------------------------------------

// --- Tab strip (non-touch, compact) ----------------------------------------
inline constexpr int kTabVisualHeight = 26;
inline constexpr int kTabVerticalSpacing = 2;
inline constexpr int kTabstripToolbarOverlap = 0;

// `LayoutConstant::kTabStripHeight` (compact, non-touch). Keep in sync with
// kTabVisualHeight, kTabVerticalSpacing, and kTabstripToolbarOverlap (no
// separate magic number).
inline constexpr int kTabStripLayoutHeightTarget =
    kTabVisualHeight + 2 * kTabVerticalSpacing + kTabstripToolbarOverlap;

// Horizontal padding inside the tab label area (see GetHorizontalTabPadding()).
inline constexpr int kTabHorizontalPadding = 4;

// Tab group header chip horizontal inset (see
// GetTabGroupTitleHorizontalInset()).
inline constexpr int kTabGroupTitleHorizontalInset = 6;

// --- Tab strip (touch + compact) --------------------------------------------
// Milder than kTabVisualHeight so touch targets stay usable.
inline constexpr int kTabVisualHeightTouchCompact = 28;
// Same as `kHorizontalTabVerticalSpacing` / non-compact spacing.
inline constexpr int kTabVerticalSpacingTouch = 4;

// --- Toolbar + URL bar (non-touch, compact) --------------------------------
// Vertical 0 keeps tab strip + toolbar within ~60 DIP when the tab strip is
// taller (e.g. kTabVisualHeight 28); row height is driven by
// kToolbarButtonHeight.
inline constexpr int kToolbarInteriorMarginVertical = 2;
inline constexpr int kToolbarInteriorMarginHorizontal = 6;
inline constexpr int kLocationBarHeight = 28;

// Omnibox inner padding (DIP) for element spacing, trailing edge, and page-info
// icon vertical padding (`LayoutConstant::kLocationBar*` padding family).
inline constexpr int kLocationBarInnerPadding = 1;

// Page info icon padding in the location bar (vertical, horizontal).
inline constexpr int kPageInfoIconPaddingVertical = 4;
inline constexpr int kPageInfoIconPaddingHorizontal = 6;

// --- Tab drag handle (non-touch, compact) -----------------------------------
// See BraveTabStyle::GetDragHandleExtension in tab_style.cc.
inline constexpr int kDragHandleExtension = 2;
inline constexpr int kDragHandleExtensionDefault = 4;

// --- Horizontal tabs: defaults when compact flag is off --------------------
inline constexpr int kTabVisualHeightDefault = 32;
inline constexpr int kTabVerticalSpacingDefault = 4;
inline constexpr int kTabstripToolbarOverlapDefault = 1;
inline constexpr int kTabHorizontalPaddingDefault = 8;
inline constexpr int kTabGroupTitleHorizontalInsetDefault = 10;

inline constexpr int kToolbarInteriorMarginVerticalDefault = 4;
inline constexpr int kToolbarInteriorMarginHorizontalDefault = 6;
inline constexpr int kLocationBarHeightDefault = 32;
inline constexpr int kLocationBarInnerPaddingDefault = 2;
inline constexpr int kPageInfoIconPaddingVerticalDefault = 6;

}  // namespace compact_horizontal_tabs_layout
}  // namespace tabs

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_BRAVE_COMPACT_HORIZONTAL_TABS_LAYOUT_H_
