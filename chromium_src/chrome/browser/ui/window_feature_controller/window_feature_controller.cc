/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/window_feature_controller/window_feature_controller.h"

#include "brave/browser/ui/views/tabs/vertical_tab_controller.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_MAC)
#define UsesImmersiveFullscreenMode UsesImmersiveFullscreenMode_ChromiumImpl
#define UsesImmersiveFullscreenTabbedMode \
  UsesImmersiveFullscreenTabbedMode_ChromiumImpl
#endif

#include <chrome/browser/ui/window_feature_controller/window_feature_controller.cc>

#if BUILDFLAG(IS_MAC)
#undef UsesImmersiveFullscreenMode
#undef UsesImmersiveFullscreenTabbedMode

bool WindowFeatureController::UsesImmersiveFullscreenMode() const {
  // Disable immersive when vertical tabs were on at startup: overlay_widget_ is
  // not created in that case, so immersive would crash. The first call happens
  // during BrowserView construction (before the user can toggle vertical tabs),
  // so this captures the startup state.
  if (!vertical_tabs_on_at_startup_.has_value()) {
    vertical_tabs_on_at_startup_ =
        vertical_tab_controller_->ShouldShowBraveVerticalTabs();
  }
  if (*vertical_tabs_on_at_startup_) {
    return false;
  }
  // Immersive is also incompatible with vertical tabs at runtime.
  if (vertical_tab_controller_->ShouldShowBraveVerticalTabs()) {
    return false;
  }

  return WindowFeatureController::UsesImmersiveFullscreenMode_ChromiumImpl();
}

bool WindowFeatureController::UsesImmersiveFullscreenTabbedMode() const {
  if (vertical_tab_controller_->ShouldShowBraveVerticalTabs()) {
    return false;
  }

  return WindowFeatureController::
      UsesImmersiveFullscreenTabbedMode_ChromiumImpl();
}
#endif

bool WindowFeatureController::NormalBrowserSupportsWindowFeature(
    WindowFeature feature,
    bool check_can_support) const {
#if BUILDFLAG(IS_WIN)
  if (feature == WindowFeature::kFeatureTitleBar) {
    // In case of vertical tab strip is allowed, we need to have ability to
    // show title bar on Windows.
    return vertical_tab_controller_->ShouldShowBraveVerticalTabs() &&
           vertical_tab_controller_->ShouldShowWindowTitleForVerticalTabs();
  }
#endif

  return WindowFeatureController::
      NormalBrowserSupportsWindowFeature_ChromiumImpl(feature,
                                                      check_can_support);
}
