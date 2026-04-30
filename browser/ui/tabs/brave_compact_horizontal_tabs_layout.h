/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_BRAVE_COMPACT_HORIZONTAL_TABS_LAYOUT_H_
#define BRAVE_BROWSER_UI_TABS_BRAVE_COMPACT_HORIZONTAL_TABS_LAYOUT_H_

#include "chrome/browser/ui/layout_constants.h"

namespace tabs {
namespace compact_horizontal_tabs_layout {

// ---------------------------------------------------------------------------
// Single place to tweak `#brave-compact-horizontal-tabs` layout (DIP).
//
// Compact metrics are applied together with
// `ShouldUseCompactHorizontalTabsForNonTouchUI()` in layout_constants.cc (flag
// on AND touch UI off). Goal: ~60px tab strip + toolbar row. When touch UI is
// active, the non-compact defaults below apply so touch targets stay usable.
// ---------------------------------------------------------------------------

// --- Tab strip (non-touch, compact) ----------------------------------------
inline constexpr int kTabVisualHeight = 26;
inline constexpr int kTabVerticalSpacing = 2;
inline constexpr int kTabstripToolbarOverlap = 8;

// Vertical delta (DIP) applied to tab strip control button placement math
// (see `UpdateButtonBorders()` in horizontal_tab_strip_region_view.cc, which
// positions the new tab button, combo button, tab search button, etc.).
//
// Why this is separate from `kTabstripToolbarOverlap`:
//   Upstream uses `LayoutConstant::kTabstripToolbarOverlap` for two roles in
//   compact mode:
//     1) Tab/toolbar geometry (kTabHeight, kTabStripHeight, tab rendering,
//        tab group underline, frame view top-area math, etc.) - wants 8.
//     2) Control button vertical placement inside the tab strip - wants the
//        smaller delta below so caption/new-tab controls stay centered.
//   We satisfy (1) centrally via
//   `GetBraveLayoutConstant(kTabstripToolbarOverlap)` and satisfy (2) via
//   per-translation-unit `GetLayoutConstant` wrappers in chromium_src that
//   redirect just the `kTabstripToolbarOverlap` cases to
//   `tabs::GetHorizontalTabControlsDelta()` (see the wrappers in
//   horizontal_tab_strip_region_view.cc and browser_frame_view_win.cc).
inline constexpr int kTabStripControlsHeightDelta = -5;

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

// Split-view pair tile on the horizontal tab strip (see
// BraveTabContainer::PaintBoundingBoxForSplitTab). Smaller than the default
// 12 DIP (--leo-radius-l) to match compact tab pills.
inline constexpr float kHorizontalSplitViewTileCornerRadiusDip = 9.f;

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
inline constexpr int kTabVerticalSpacingDefault = kHorizontalTabVerticalSpacing;
inline constexpr int kTabstripToolbarOverlapDefault = 1;
inline constexpr int kTabStripControlsHeightDeltaDefault = -4;
inline constexpr int kTabHorizontalPaddingDefault = 8;
inline constexpr int kTabGroupTitleHorizontalInsetDefault = 10;

inline constexpr int kToolbarInteriorMarginVerticalDefault = 4;
inline constexpr int kToolbarInteriorMarginHorizontalDefault = 6;
inline constexpr int kLocationBarHeightDefault = 32;
inline constexpr int kLocationBarInnerPaddingDefault = 2;
inline constexpr int kPageInfoIconPaddingVerticalDefault = 6;

}  // namespace compact_horizontal_tabs_layout

// Returns true iff `#brave-compact-horizontal-tabs` is enabled AND touch UI is
// off, i.e. the gate that selects the compact metrics in
// `compact_horizontal_tabs_layout` over the defaults. Centralised here so all
// consumers share a single gating helper instead of re-implementing the
// `IsEnabled(kBraveCompactHorizontalTabs) && !touch_ui()` check.
bool ShouldUseCompactHorizontalTabsForNonTouchUI();

}  // namespace tabs

#endif  // BRAVE_BROWSER_UI_TABS_BRAVE_COMPACT_HORIZONTAL_TABS_LAYOUT_H_
