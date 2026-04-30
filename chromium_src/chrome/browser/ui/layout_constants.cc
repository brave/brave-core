/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/layout_constants.h"

#include <algorithm>
#include <optional>

#include "base/feature_list.h"
#include "brave/browser/ui/tabs/brave_compact_horizontal_tabs_layout.h"
#include "chrome/browser/ui/tabs/features.h"
#include "ui/base/pointer/touch_ui_controller.h"
#include "ui/gfx/geometry/insets.h"

namespace {

using tabs::HorizontalTabsUpdateEnabled;
using tabs::ShouldUseCompactHorizontalTabsForNonTouchUI;

std::optional<gfx::Insets> GetBraveLayoutInsets(LayoutInset inset) {
  const bool touch_ui = ui::TouchUiController::Get()->touch_ui();
  const bool compact = ShouldUseCompactHorizontalTabsForNonTouchUI();
  switch (inset) {
    case LOCATION_BAR_PAGE_INFO_ICON_PADDING:
      return gfx::Insets::VH(
          compact ? tabs::compact_horizontal_tabs_layout::
                        kPageInfoIconPaddingVertical
                  : tabs::compact_horizontal_tabs_layout::
                        kPageInfoIconPaddingVerticalDefault,
          tabs::compact_horizontal_tabs_layout::kPageInfoIconPaddingHorizontal);
    case LOCATION_BAR_PAGE_ACTION_ICON_PADDING:
      return gfx::Insets::VH(4, 4);
    case TOOLBAR_BUTTON:
      // Use 4 inset - (TOOLBAR_BUTTON_HEIGHT(28) - icon size(20)) / 2
      // icon size - ToolbarButton::kDefaultIconSize
      return gfx::Insets(touch_ui ? 12 : 4);
    case TOOLBAR_INTERIOR_MARGIN:
      if (touch_ui) {
        return gfx::Insets();
      }
      return compact
                 ? gfx::Insets::VH(tabs::compact_horizontal_tabs_layout::
                                       kToolbarInteriorMarginVertical,
                                   tabs::compact_horizontal_tabs_layout::
                                       kToolbarInteriorMarginHorizontal)
                 : gfx::Insets::VH(tabs::compact_horizontal_tabs_layout::
                                       kToolbarInteriorMarginVerticalDefault,
                                   tabs::compact_horizontal_tabs_layout::
                                       kToolbarInteriorMarginHorizontalDefault);
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
      if (!HorizontalTabsUpdateEnabled()) {
        return std::nullopt;
      }
      return ShouldUseCompactHorizontalTabsForNonTouchUI()
                 ? tabs::compact_horizontal_tabs_layout::kTabstripToolbarOverlap
                 : tabs::compact_horizontal_tabs_layout::
                       kTabstripToolbarOverlapDefault;
    }
    case LayoutConstant::kLocationBarChildCornerRadius:
      return 4;
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
      return ShouldUseCompactHorizontalTabsForNonTouchUI()
                 ? tabs::compact_horizontal_tabs_layout::kLocationBarHeight
                 : tabs::compact_horizontal_tabs_layout::
                       kLocationBarHeightDefault;
    case LayoutConstant::kLocationBarTrailingIconSize:
      return 18;
    case LayoutConstant::kLocationBarIconSize:
      return 16;
    case LayoutConstant::kLocationBarElementPadding:
    case LayoutConstant::kLocationBarPageInfoIconVerticalPadding:
    case LayoutConstant::kLocationBarTrailingDecorationEdgePadding:
      return ShouldUseCompactHorizontalTabsForNonTouchUI()
                 ? tabs::compact_horizontal_tabs_layout::
                       kLocationBarInnerPadding
                 : tabs::compact_horizontal_tabs_layout::
                       kLocationBarInnerPaddingDefault;
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

bool ShouldUseCompactHorizontalTabsForNonTouchUI() {
  return base::FeatureList::IsEnabled(tabs::kBraveCompactHorizontalTabs) &&
         !ui::TouchUiController::Get()->touch_ui();
}

int GetHorizontalTabHeight() {
  return ShouldUseCompactHorizontalTabsForNonTouchUI()
             ? tabs::compact_horizontal_tabs_layout::kTabVisualHeight
             : tabs::compact_horizontal_tabs_layout::kTabVisualHeightDefault;
}

int GetHorizontalTabVerticalSpacing() {
  return ShouldUseCompactHorizontalTabsForNonTouchUI()
             ? tabs::compact_horizontal_tabs_layout::kTabVerticalSpacing
             : tabs::compact_horizontal_tabs_layout::kTabVerticalSpacingDefault;
}

int GetHorizontalTabControlsDelta() {
  return ShouldUseCompactHorizontalTabsForNonTouchUI()
             ? tabs::compact_horizontal_tabs_layout::
                   kTabStripControlsHeightDelta
             : tabs::compact_horizontal_tabs_layout::
                   kTabStripControlsHeightDeltaDefault;
}

int GetHorizontalTabStripHeight() {
  return GetHorizontalTabHeight() + (GetHorizontalTabVerticalSpacing() * 2);
}

int GetHorizontalTabPadding() {
  return ShouldUseCompactHorizontalTabsForNonTouchUI()
             ? tabs::compact_horizontal_tabs_layout::kTabHorizontalPadding
             : tabs::compact_horizontal_tabs_layout::
                   kTabHorizontalPaddingDefault;
}

int GetTabGroupTitleVerticalInset() {
  return std::max(0, (GetHorizontalTabHeight() - kTabGroupLineHeight) / 2);
}

int GetTabGroupTitleHorizontalInset() {
  return ShouldUseCompactHorizontalTabsForNonTouchUI()
             ? tabs::compact_horizontal_tabs_layout::
                   kTabGroupTitleHorizontalInset
             : tabs::compact_horizontal_tabs_layout::
                   kTabGroupTitleHorizontalInsetDefault;
}

}  // namespace tabs
