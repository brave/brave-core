/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_features.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"

#include "src/chrome/browser/ui/views/side_panel/side_panel_web_ui_view.cc"

void SidePanelWebUIView::AddedToWidget() {
  if (base::FeatureList::IsEnabled(features::kBraveWebViewRoundedCorners)) {
    // Side panel WebUI views are positioned at the bottom of the side panel. In
    // order to maintain rounded corners around the side panel, give the web
    // contents native view rounded corners on the bottom.
    constexpr auto radius = BraveContentsViewUtil::kBorderRadius;
    holder()->SetCornerRadii(gfx::RoundedCornersF(0, 0, radius, radius));
  }
}
