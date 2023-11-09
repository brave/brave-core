/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_group_style.h"

#include "brave/browser/ui/tabs/brave_tab_layout_constants.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/tabs/brave_tab_group_underline.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"

#define TabGroupUnderline BraveTabGroupUnderline
#define TabGroupStyle TabGroupStyle_ChromiumImpl
#define ChromeRefresh2023TabGroupStyle \
  ChromeRefresh2023TabGroupStyle_ChromiumImpl

#include "src/chrome/browser/ui/views/tabs/tab_group_style.cc"

#undef ChromeRefresh2023TabGroupStyle
#undef TabGroupStyle
#undef TabGroupUnderline

const int TabGroupStyle::kStrokeThicknessForVerticalTabs = 4;

SkPath TabGroupStyle::GetUnderlinePath(gfx::Rect local_bounds) const {
  if (!ShouldShowVerticalTabs()) {
    return TabGroupStyle_ChromiumImpl::GetUnderlinePath(local_bounds);
  }

  // In vertical tabs, underline is not actually "underline'. It's vertical line
  // at the left side of the tab group. And it has half rounded corners.
  //
  // +   group header   | '+' is the underline.
  // ++  tab 1          | Drawing starts from top-right and goes
  // ++  tab 2          | counter-clockwise
  // +   tab 3          |
  //
  SkPath path;
  path.arcTo(/* rx = */ kStrokeThicknessForVerticalTabs,
             /* ry = */ kStrokeThicknessForVerticalTabs,
             /* angle = */ 180.f, SkPath::kSmall_ArcSize, SkPathDirection::kCW,
             /* x = */ kStrokeThicknessForVerticalTabs,
             /* y = */ kStrokeThicknessForVerticalTabs);
  path.lineTo(kStrokeThicknessForVerticalTabs,
              local_bounds.height() - kStrokeThicknessForVerticalTabs);
  path.arcTo(/* rx = */ kStrokeThicknessForVerticalTabs,
             /* ry = */ kStrokeThicknessForVerticalTabs,
             /* angle = */ 180.f, SkPath::kSmall_ArcSize, SkPathDirection::kCW,
             /* x = */ 0,
             /* y = */ local_bounds.height());
  path.close();

  return path;
}

gfx::Insets TabGroupStyle::GetInsetsForHeaderChip(
    bool should_show_sync_icon) const {
  auto insets =
      TabGroupStyle_ChromiumImpl::GetInsetsForHeaderChip(should_show_sync_icon);
  if (!tabs::features::HorizontalTabsUpdateEnabled()) {
    return insets;
  }
  if (!ShouldShowVerticalTabs()) {
    insets.set_top(brave_tabs::kTabGroupTitleVerticalInset)
        .set_bottom(brave_tabs::kTabGroupTitleVerticalInset);
  }
  return insets;
}

bool TabGroupStyle::ShouldShowVerticalTabs() const {
  return tabs::utils::ShouldShowVerticalTabs(tab_group_views_->GetBrowser());
}

float TabGroupStyle::GetEmptyChipSize() const {
  if (!tabs::features::HorizontalTabsUpdateEnabled()) {
    return TabGroupStyle_ChromiumImpl::GetEmptyChipSize();
  }
  return brave_tabs::kEmptyGroupTitleSize;
}

int TabGroupStyle::GetChipCornerRadius() const {
  if (!tabs::features::HorizontalTabsUpdateEnabled()) {
    return TabGroupStyle_ChromiumImpl::GetChipCornerRadius();
  }
  return brave_tabs::kTabBorderRadius;
}

int ChromeRefresh2023TabGroupStyle::GetTabGroupOverlapAdjustment() {
  return ChromeRefresh2023TabGroupStyle_ChromiumImpl::
      GetTabGroupOverlapAdjustment();
}
