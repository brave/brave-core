/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "build/build_config.h"

#include <chrome/browser/ui/window_feature_controller/window_feature_controller.cc>

bool WindowFeatureController::NormalBrowserSupportsWindowFeature(
    WindowFeature feature,
    bool check_can_support) const {
#if BUILDFLAG(IS_WIN)
  if (feature == WindowFeature::kFeatureTitleBar) {
    // In case of vertical tab strip is allowed, we need to have ability to
    // show title bar on Windows.
    return tabs::utils::ShouldShowBraveVerticalTabs(&browser_.get()) &&
           tabs::utils::ShouldShowWindowTitleForVerticalTabs(&browser_.get());
  }
#endif

  return WindowFeatureController::
      NormalBrowserSupportsWindowFeature_ChromiumImpl(feature,
                                                      check_can_support);
}
