/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_tab_layout_constants.h"
#include "brave/browser/ui/tabs/features.h"
#include "chrome/browser/ui/layout_constants.h"
#include "ui/base/ui_base_features.h"

namespace features {

bool BraveHorizontalTabsUpdateEnabled() {
  return tabs::features::HorizontalTabsUpdateEnabled();
}

}  // namespace features

namespace {

int BraveGetLayoutConstant(LayoutConstant layout_constant) {
  if (layout_constant == TAB_STRIP_PADDING &&
      tabs::features::HorizontalTabsUpdateEnabled()) {
    return brave_tabs::kHorizontalTabVerticalSpacing;
  }
  return GetLayoutConstant(layout_constant);
}

}  // namespace

// When updated horizontal tabs are enabled, we want to use the same layout
// logic as upstream's "Refresh2023" for tab strip positioning and for the
// window caption button height. When upstream's feature flag is removed, this
// define can also be removed.
#define IsChromeRefresh2023 BraveHorizontalTabsUpdateEnabled

// Upstream was modified to use `TAB_STRIP_PADDING` when calculating the amount
// of space to dedicate to the frame resize handle above the tabstrip. When the
// "Refresh2023" flag is not enabled, this value is zero. Override
// `GetLayoutConstant` to return a non-zero value for this constant.
#define GetLayoutConstant BraveGetLayoutConstant

#include "src/chrome/browser/ui/views/frame/browser_frame_view_win.cc"  // IWYU pragma: export

#undef GetLayoutConstant
#undef IsChromeRefresh2023
