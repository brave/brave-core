/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/tabs/tab_style.h"

#include "brave/browser/ui/tabs/brave_tab_layout_constants.h"
#include "brave/browser/ui/tabs/features.h"
#include "chrome/browser/ui/layout_constants.h"

namespace {

// A subclass of TabStyle used to customize tab layout and visuals to support
// Brave specifics including horizontal tabs.
class BraveTabStyle : public TabStyle {
 public:
  int GetTabOverlap() const override {
    if (!tabs::features::HorizontalTabsUpdateEnabled()) {
      return TabStyle::GetTabOverlap();
    }
    return brave_tabs::kHorizontalTabOverlap;
  }

  int GetTopCornerRadius() const override {
    if (!tabs::features::HorizontalTabsUpdateEnabled()) {
      return TabStyle::GetTopCornerRadius();
    }
    return brave_tabs::kTabBorderRadius;
  }

  int GetBottomCornerRadius() const override {
    if (!tabs::features::HorizontalTabsUpdateEnabled()) {
      return TabStyle::GetBottomCornerRadius();
    }
    return brave_tabs::kTabBorderRadius;
  }

  gfx::Insets GetContentsInsets() const override {
    if (!tabs::features::HorizontalTabsUpdateEnabled()) {
      return TabStyle::GetContentsInsets();
    }
    return gfx::Insets::VH(0, brave_tabs::GetHorizontalTabPadding() +
                                  brave_tabs::kHorizontalTabInset);
  }

  // TODO(https://github.com/brave/brave-browser/issues/46000): It is not very
  // clear if this is the best implementation with the [SxS] being still under
  // development in upstream.
  int GetPinnedWidth(const bool is_split) const override {
    if (!is_split) {
      return TabStyle::GetPinnedWidth(is_split);
    }
    return brave_tabs::GetHorizontalTabHeight() +
           brave_tabs::kHorizontalTabInset * 2;
  }

  int GetDragHandleExtension(int height) const override {
    if (!tabs::features::HorizontalTabsUpdateEnabled()) {
      return TabStyle::GetDragHandleExtension(height);
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
};

}  // namespace

#define BRAVE_TAB_STYLE_GET return new BraveTabStyle();
#include "src/chrome/browser/ui/tabs/tab_style.cc"
#undef BRAVE_TAB_STYLE_GET
