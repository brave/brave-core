/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/layout_constants.h"

#include <algorithm>
#include <optional>

#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/tabs/features.h"
#include "components/prefs/pref_service.h"
#include "ui/base/pointer/touch_ui_controller.h"
#include "ui/gfx/geometry/insets.h"

namespace {

using tabs::HorizontalTabsUpdateEnabled;
using tabs::UseCompactHorizontalTabs;

std::optional<gfx::Insets> GetBraveLayoutInsets(LayoutInset inset) {
  const bool touch_ui = ui::TouchUiController::Get()->touch_ui();
  const bool compact = UseCompactHorizontalTabs();
  switch (inset) {
    case LOCATION_BAR_PAGE_INFO_ICON_PADDING:
      return gfx::Insets::VH(compact ? 4 : 6, 6);
    case LOCATION_BAR_PAGE_ACTION_ICON_PADDING:
      // Trailing icons are 20px. Compact location_height is 26, so vertical
      // padding must be 3 (20 + 2*3 = 26). Normal location_height is 28
      // (20 + 2*4 = 28).
      return gfx::Insets::VH(compact ? 3 : 4, 4);
    case TOOLBAR_BUTTON:
      // Use 4 inset - (TOOLBAR_BUTTON_HEIGHT(28) - icon size(20)) / 2
      // icon size - ToolbarButton::kDefaultIconSize
      return gfx::Insets(touch_ui ? 12 : 4);
    case TOOLBAR_INTERIOR_MARGIN:
      if (touch_ui) {
        return gfx::Insets();
      }
      return gfx::Insets::VH(compact ? 2 : 4, 6);
    default:
      break;
  }
  return std::nullopt;
}

// Returns a |nullopt| if the UI color is not handled by Brave.
std::optional<int> GetBraveLayoutConstant(LayoutConstant constant) {
  const bool touch = ui::TouchUiController::Get()->touch_ui();
  switch (constant) {
    case LayoutConstant::kTabHeight: {
      if (HorizontalTabsUpdateEnabled()) {
        return tabs::GetHorizontalTabHeight();
      }
      return (touch ? 41 : 30) +
             GetLayoutConstant(LayoutConstant::kTabstripToolbarOverlap);
    }
    case LayoutConstant::kTabStripHeight: {
      if (HorizontalTabsUpdateEnabled()) {
        return tabs::GetHorizontalTabStripHeight() +
               GetLayoutConstant(LayoutConstant::kTabstripToolbarOverlap);
      }
      return std::nullopt;
    }
    case LayoutConstant::kTabStripPadding: {
      if (HorizontalTabsUpdateEnabled()) {
        return tabs::GetHorizontalTabVerticalSpacing();
      }
      return std::nullopt;
    }
    case LayoutConstant::kTabstripToolbarOverlap: {
      // Upstream extends tabs 1px into the toolbar to hide a fractional-scale
      // gap at the tab/toolbar seam. Brave's tabs are free-floating pills with
      // a deliberate gap below them, so there is no seam and no need for an
      // overlap.
      if (HorizontalTabsUpdateEnabled()) {
        return 0;
      }
      return std::nullopt;
    }
    case LayoutConstant::kLocationBarChildCornerRadius:
      return 6;
    // Uniform padding keeps the LHS camera/mic chip square. Match site-info
    // width in normal (16 + 2*6 = 28); use 4 in compact so the chip fits
    // location_height (16 + 2*4 = 24 ≤ 26).
    case LayoutConstant::kLocationBarChipPadding:
      return UseCompactHorizontalTabs() ? 4 : 6;
    case LayoutConstant::kTabSeparatorHeight: {
      return 16;
    }
    case LayoutConstant::kToolbarButtonHeight: {
      // See also SidebarButtonView::kSidebarButtonSize
      return touch ? 48 : 28;
    }
    case LayoutConstant::kToolbarCornerRadius:
      return 8;

    case LayoutConstant::kLocationBarHeight:
      // Consider adjust below element padding also when this height is changed.
      return UseCompactHorizontalTabs() ? 28 : 32;
    case LayoutConstant::kLocationBarElementPadding:
    case LayoutConstant::kLocationBarPageInfoIconVerticalPadding:
      return UseCompactHorizontalTabs() ? 1 : 2;
    // Flush trailing icons to the omnibar's right edge (was 2 normal / 1
    // compact).
    case LayoutConstant::kLocationBarTrailingDecorationEdgePadding:
      return 0;
    default:
      break;
  }
  return std::nullopt;
}

}  // namespace

// Forward declaration
gfx::Insets GetLayoutInsets_ChromiumImpl(LayoutInset inset);

#define LayoutInset LayoutInset inset) {           \
    const std::optional<gfx::Insets> braveOption = \
        GetBraveLayoutInsets(inset);               \
    if (braveOption) {                             \
      return braveOption.value();                  \
    }                                              \
                                                   \
    return GetLayoutInsets_ChromiumImpl(inset);    \
  }                                                \
                                                   \
  gfx::Insets GetLayoutInsets_ChromiumImpl(LayoutInset

// Use our own version of GetLayoutConstant to include Brave values.
#define GetLayoutConstant GetLayoutConstant_ChromiumImpl
#include <chrome/browser/ui/layout_constants.cc>
#undef GetLayoutConstant
#undef LayoutInset

int GetLayoutConstant(LayoutConstant constant) {
  const std::optional<int> brave_option = GetBraveLayoutConstant(constant);
  if (brave_option) {
    return brave_option.value();
  }
  return GetLayoutConstant_ChromiumImpl(constant);
}

namespace tabs {

int GetHorizontalTabHeight() {
  return UseCompactHorizontalTabs() ? 26 : 32;
}

int GetHorizontalTabVerticalSpacing() {
  return UseCompactHorizontalTabs() ? 2 : 4;
}

int GetHorizontalTabStripHeight() {
  return GetHorizontalTabHeight() + (GetHorizontalTabVerticalSpacing() * 2);
}

int GetHorizontalTabPadding() {
  return UseCompactHorizontalTabs() ? 4 : 8;
}

int GetTabGroupTitleVerticalInset() {
  return std::max(0, (GetHorizontalTabHeight() - kTabGroupLineHeight) / 2);
}

int GetTabGroupTitleHorizontalInset() {
  return UseCompactHorizontalTabs() ? 6 : 10;
}

int GetDragHandleExtensionHeight() {
  return UseCompactHorizontalTabs() ? 2 : 4;
}

bool UseCompactHorizontalTabs() {
  if (ui::TouchUiController::Get()->touch_ui()) {
    return false;
  }
  if (g_browser_process && g_browser_process->local_state()) {
    return g_browser_process->local_state()->GetBoolean(
        brave_tabs::kCompactHorizontalTabs);
  }
  return false;
}

}  // namespace tabs
