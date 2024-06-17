/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_BRAVE_TAB_STYLE_H_
#define BRAVE_BROWSER_UI_TABS_BRAVE_TAB_STYLE_H_

#include "brave/browser/ui/tabs/brave_tab_layout_constants.h"
#include "brave/browser/ui/tabs/features.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/layout_constants.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/geometry/insets.h"

// A subclass of TabStyle used to customize tab layout and visuals. It is
// implemented as a template because it must be included in the source file
// override before the base class definition.
template <typename TabStyleBase>
class BraveTabStyle : public TabStyleBase {
 public:
  int GetTabOverlap() const override {
    if (!tabs::features::HorizontalTabsUpdateEnabled()) {
      return TabStyleBase::GetTabOverlap();
    }
    return brave_tabs::kHorizontalTabOverlap;
  }

  int GetTopCornerRadius() const override {
    if (!tabs::features::HorizontalTabsUpdateEnabled()) {
      return TabStyleBase::GetTopCornerRadius();
    }
    return brave_tabs::kTabBorderRadius;
  }

  int GetBottomCornerRadius() const override {
    if (!tabs::features::HorizontalTabsUpdateEnabled()) {
      return TabStyleBase::GetBottomCornerRadius();
    }
    return brave_tabs::kTabBorderRadius;
  }

  gfx::Insets GetContentsInsets() const override {
    if (!tabs::features::HorizontalTabsUpdateEnabled()) {
      return TabStyleBase::GetContentsInsets();
    }
    return gfx::Insets::VH(
        0, brave_tabs::kHorizontalTabPadding + brave_tabs::kHorizontalTabInset);
  }

  int GetPinnedWidth() const override {
    if (!tabs::features::HorizontalTabsUpdateEnabled()) {
      return TabStyleBase::GetPinnedWidth();
    }
    return brave_tabs::kHorizontalTabHeight +
           brave_tabs::kHorizontalTabInset * 2;
  }

  int GetDragHandleExtension(int height) const override {
    if (!tabs::features::HorizontalTabsUpdateEnabled()) {
      return TabStyleBase::GetDragHandleExtension(height);
    }
    // The "drag handle extension" is the amount of space in DIP at the top of
    // inactive tabs where mouse clicks are treated as clicks in the "caption"
    // area, i.e. the draggable part of the window frame.
    return 4;
  }

  gfx::Size GetSeparatorSize() const override {
    return gfx::Size(1, GetLayoutConstant(TAB_SEPARATOR_HEIGHT));
  }

  gfx::Insets GetSeparatorMargins() const override {
    return gfx::Insets::TLBR(0, GetSeparatorSize().width() * -1, 0,
                             GetSeparatorSize().width() * -1);
  }

  int GetSeparatorCornerRadius() const override { return 0; }

  SkColor GetTabBackgroundColor(
      const TabStyleBase::TabSelectionState state,
      bool hovered,
      const bool frame_active,
      const ui::ColorProvider& color_provider) const override {
    const SkColor active_color = color_provider.GetColor(
        frame_active ? kColorTabBackgroundActiveFrameActive
                     : kColorTabBackgroundActiveFrameInactive);
    const SkColor inactive_color = color_provider.GetColor(
        frame_active ? kColorTabBackgroundInactiveFrameActive
                     : kColorTabBackgroundInactiveFrameInactive);

    if (hovered) {
      return active_color;
    }

    switch (state) {
      case TabStyleBase::TabSelectionState::kActive:
        return active_color;
      case TabStyleBase::TabSelectionState::kSelected:
        return color_utils::AlphaBlend(active_color, inactive_color,
                                       TabStyleBase::GetSelectedTabOpacity());
      case TabStyleBase::TabSelectionState::kInactive:
        return inactive_color;
      default:
        NOTREACHED_NORETURN();
    }
  }
};

#endif  // BRAVE_BROWSER_UI_TABS_BRAVE_TAB_STYLE_H_
