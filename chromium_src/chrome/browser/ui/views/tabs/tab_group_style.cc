/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_group_style.h"

#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"

#define TabGroupStyle TabGroupStyle_ChromiumImpl
#define ChromeRefresh2023TabGroupStyle \
  ChromeRefresh2023TabGroupStyle_ChromiumImpl

#include "src/chrome/browser/ui/views/tabs/tab_group_style.cc"
#undef ChromeRefresh2023TabGroupStyle
#undef TabGroupStyle

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

bool TabGroupStyle::ShouldShowVerticalTabs() const {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs)) {
    return false;
  }

  return tabs::utils::ShouldShowVerticalTabs(tab_group_views_->GetBrowser());
}

int ChromeRefresh2023TabGroupStyle::GetTabGroupOverlapAdjustment() {
  return ChromeRefresh2023TabGroupStyle_ChromiumImpl::
      GetTabGroupOverlapAdjustment();
}
