/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "brave/browser/ui/tabs/brave_tab_layout_constants.h"
#include "brave/browser/ui/tabs/features.h"
#include "chrome/browser/ui/layout_constants.h"
#include "ui/base/pointer/touch_ui_controller.h"
#include "ui/gfx/geometry/insets.h"

namespace {

using tabs::features::HorizontalTabsUpdateEnabled;

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
    case TAB_HEIGHT: {
      if (HorizontalTabsUpdateEnabled()) {
        return brave_tabs::GetHorizontalTabHeight();
      }
      return (touch ? 41 : 30) + GetLayoutConstant(TABSTRIP_TOOLBAR_OVERLAP);
    }
    case TAB_STRIP_HEIGHT: {
      if (HorizontalTabsUpdateEnabled()) {
        return brave_tabs::GetHorizontalTabStripHeight() +
               GetLayoutConstant(TABSTRIP_TOOLBAR_OVERLAP);
      }
      return std::nullopt;
    }
    case TAB_STRIP_PADDING: {
      if (HorizontalTabsUpdateEnabled()) {
        return brave_tabs::kHorizontalTabVerticalSpacing;
      }
      return std::nullopt;
    }
    case TABSTRIP_TOOLBAR_OVERLAP: {
      if (!HorizontalTabsUpdateEnabled()) {
        return std::nullopt;
      }
      return 1;
    }
    case LOCATION_BAR_CHILD_CORNER_RADIUS:
      return 4;
    case TAB_SEPARATOR_HEIGHT: {
      return 16;
    }
    case TOOLBAR_BUTTON_HEIGHT: {
      // See also SidebarButtonView::kSidebarButtonSize
      return touch ? 48 : 28;
    }
    case TOOLBAR_CORNER_RADIUS:
      return 8;

    case LOCATION_BAR_HEIGHT:
      // Consider adjust below element padding also when this height is changed.
      return 32;
    case LOCATION_BAR_TRAILING_ICON_SIZE:
      return 18;
    case LOCATION_BAR_ICON_SIZE:
      return 16;
    case LOCATION_BAR_ELEMENT_PADDING:
    case LOCATION_BAR_PAGE_INFO_ICON_VERTICAL_PADDING:
    case LOCATION_BAR_TRAILING_DECORATION_EDGE_PADDING:
      return 2;
    default:
      break;
  }
  return std::nullopt;
}

}  // namespace

// Forward declaration
int GetLayoutConstant_ChromiumImpl(LayoutConstant constant);
gfx::Insets GetLayoutInsets_ChromiumImpl(LayoutInset inset);

#define LayoutConstant LayoutConstant constant) {                            \
    const std::optional<int> braveOption = GetBraveLayoutConstant(constant); \
    if (braveOption) {                                                       \
      return braveOption.value();                                            \
    }                                                                        \
                                                                             \
    return GetLayoutConstant_ChromiumImpl(constant);                         \
  }                                                                          \
                                                                             \
  int GetLayoutConstant_ChromiumImpl(LayoutConstant

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
#undef LayoutConstant
