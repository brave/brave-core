/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/layout_constants.h"

#include <optional>

#include "chrome/browser/ui/tabs/features.h"
#include "ui/base/pointer/touch_ui_controller.h"
#include "ui/gfx/geometry/insets.h"

namespace {

using tabs::HorizontalTabsUpdateEnabled;

std::optional<gfx::Insets> GetBraveLayoutInsets(LayoutInset inset) {
  const bool touch_ui = ui::TouchUiController::Get()->touch_ui();
  switch (inset) {
    case LOCATION_BAR_PAGE_INFO_ICON_PADDING:
      return gfx::Insets::VH(6, 6);
    case LOCATION_BAR_PAGE_ACTION_ICON_PADDING:
      return gfx::Insets::VH(4, 4);
    case TOOLBAR_BUTTON:
      // Use 4 inset - (TOOLBAR_BUTTON_HEIGHT(28) - icon size(20)) / 2
      // icon size - ToolbarButton::kDefaultIconSize
      return gfx::Insets(touch_ui ? 12 : 4);
    case TOOLBAR_INTERIOR_MARGIN:
      return touch_ui ? gfx::Insets() : gfx::Insets::VH(4, 8);
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
        return tabs::kHorizontalTabVerticalSpacing;
      }
      return std::nullopt;
    }
    case LayoutConstant::kTabstripToolbarOverlap: {
      if (!HorizontalTabsUpdateEnabled()) {
        return std::nullopt;
      }
      return 1;
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
      return 32;
    case LayoutConstant::kLocationBarTrailingIconSize:
      return 18;
    case LayoutConstant::kLocationBarIconSize:
      return 16;
    case LayoutConstant::kLocationBarElementPadding:
    case LayoutConstant::kLocationBarPageInfoIconVerticalPadding:
    case LayoutConstant::kLocationBarTrailingDecorationEdgePadding:
      return 2;
    default:
      break;
  }
  return std::nullopt;
}

}  // namespace

// Forward declaration
gfx::Insets GetLayoutInsets_ChromiumImpl(LayoutInset inset);

#define BRAVE_LAYOUT_CONSTANTS_GET_LAYOUT_CONSTANT                          \
  const std::optional<int> brave_option = GetBraveLayoutConstant(constant); \
  if (brave_option) {                                                       \
    return brave_option.value();                                            \
  }

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

#include <chrome/browser/ui/layout_constants.cc>
#undef LayoutInset
#undef BRAVE_LAYOUT_CONSTANTS_GET_LAYOUT_CONSTANT

namespace tabs {

namespace {

bool UseCompact() {
  return base::FeatureList::IsEnabled(tabs::kBraveCompactHorizontalTabs);
}

}  // namespace

int GetHorizontalTabHeight() {
  return UseCompact() ? 28 : 32;
}

int GetHorizontalTabStripHeight() {
  return GetHorizontalTabHeight() + (kHorizontalTabVerticalSpacing * 2);
}

int GetHorizontalTabPadding() {
  return UseCompact() ? 4 : 8;
}

int GetTabGroupTitleVerticalInset() {
  return (GetHorizontalTabHeight() - kTabGroupLineHeight) / 2;
}

int GetTabGroupTitleHorizontalInset() {
  return UseCompact() ? 6 : 10;
}

}  // namespace tabs
