// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/tabs/public/vertical_tab_controller.h"
#include "chrome/browser/ui/window_feature_controller/window_feature_controller.h"

// Report the title bar as supported when the browser supports the vertical tab
// strip, so the constructor creates the `window_title_` label. The label is
// needed even when the title bar is hidden at creation time, as users can
// switch tab orientation or title bar visibility through preferences.
#define SupportsWindowFeature(feature)                                        \
  SupportsWindowFeature(feature) ||                                           \
      (feature == WindowFeatureController::WindowFeature::kFeatureTitleBar && \
       VerticalTabController::From(browser) &&                                \
       VerticalTabController::From(browser)->SupportsBraveVerticalTabs())

#include <chrome/browser/ui/views/frame/browser_frame_view_win.cc>

#undef SupportsWindowFeature
