/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_BRAVE_TAB_STYLE_H_
#define BRAVE_BROWSER_UI_TABS_BRAVE_TAB_STYLE_H_

#include "brave/browser/ui/tabs/brave_tab_layout_constants.h"
#include "brave/browser/ui/tabs/features.h"
#include "chrome/browser/ui/layout_constants.h"
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
    const int shape_height = GetLayoutConstant(TAB_HEIGHT) -
                             brave_tabs::kHorizontalTabVerticalSpacing * 2;
    return shape_height + brave_tabs::kHorizontalTabInset * 2;
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
};

#endif  // BRAVE_BROWSER_UI_TABS_BRAVE_TAB_STYLE_H_
