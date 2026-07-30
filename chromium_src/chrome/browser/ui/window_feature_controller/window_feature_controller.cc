/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/window_feature_controller/window_feature_controller.h"

#include "build/build_config.h"

// Forward declared to avoid adding a compile-time dependency.
// impl target is //brave/browser/ui/window_feature_controller:chromium_impl.
bool BraveDisablesImmersiveFullscreenMode(
    const base::WeakPtr<VerticalTabController>& vertical_tab_controller);
bool BraveShouldShowTitlebar(
    const base::WeakPtr<VerticalTabController>& vertical_tab_controller);

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
  // Disable immersive when vertical tabs(or compact mode) were on at startup:
  // overlay_widget_ is not created in that case, so immersive would crash. The
  // first call happens during BrowserView construction (before the user can
  // toggle vertical tabs), so this captures the startup state.
  if (!disabled_at_startup_.has_value()) {
    disabled_at_startup_ =
        BraveDisablesImmersiveFullscreenMode(vertical_tab_controller_);
  }

  if (*disabled_at_startup_) {
    return false;
  }

  // Immersive is also incompatible with vertical tabs at runtime.
  if (BraveDisablesImmersiveFullscreenMode(vertical_tab_controller_)) {
    return false;
  }

  return WindowFeatureController::UsesImmersiveFullscreenMode_ChromiumImpl();
}

bool WindowFeatureController::UsesImmersiveFullscreenTabbedMode() const {
  if (BraveDisablesImmersiveFullscreenMode(vertical_tab_controller_)) {
    return false;
  }

  return WindowFeatureController::
      UsesImmersiveFullscreenTabbedMode_ChromiumImpl();
}
#endif
