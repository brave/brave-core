/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/browser_window/public/browser_window_features.h"

#include "base/notreached.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel_coordinator.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/ui/brave_vpn/brave_vpn_controller.h"
#endif

#if !BUILDFLAG(ENABLE_BRAVE_VPN)
// Use stub class to avoid incomplete type build error.
class BraveVPNController {};
#endif

#define SidePanelCoordinator BraveSidePanelCoordinator
#define InitPostBrowserViewConstruction \
  InitPostBrowserViewConstruction_ChromiumImpl

#include "src/chrome/browser/ui/browser_window/browser_window_features.cc"

#undef InitPostBrowserViewConstruction
#undef SidePanelCoordinator

void BrowserWindowFeatures::InitPostBrowserViewConstruction(
    BrowserView* browser_view) {
  InitPostBrowserViewConstruction_ChromiumImpl(browser_view);

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  brave_vpn_controller_ = std::make_unique<BraveVPNController>(browser_view);
#endif
}

BraveVPNController* BrowserWindowFeatures::GetBraveVPNController() {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  return brave_vpn_controller_.get();
#else
  NOTREACHED_NORETURN();
#endif
}
