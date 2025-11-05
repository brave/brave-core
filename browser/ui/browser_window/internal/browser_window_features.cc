/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/browser_window/public/browser_window_features.h"

#include "base/memory/ptr_util.h"
#include "base/notreached.h"
#include "build/build_config.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/brave_browser_window.h"
#endif  // !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/brave_rewards/rewards_panel_coordinator.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/tabs/features.h"
#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/views/side_panel/playlist/playlist_side_panel_coordinator.h"
#endif  // !BUILDFLAG(IS_ANDROID)
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/playlist/core/common/features.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/views/frame/browser_view.h"
#else
class BrowserView;
#endif  // !BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/ui/brave_vpn/brave_vpn_controller.h"
#endif

#if !BUILDFLAG(ENABLE_BRAVE_VPN)
// Use stub class to avoid incomplete type build error.
class BraveVPNController {};
#endif

#if BUILDFLAG(IS_ANDROID)
// Stub class to avoid incomplete type build error with std::unique_ptr
class PlaylistSidePanelCoordinator {};
#endif  // BUILDFLAG(IS_ANDROID)

BrowserWindowFeatures::BrowserWindowFeatures() = default;
BrowserWindowFeatures::~BrowserWindowFeatures() = default;

BraveVPNController* BrowserWindowFeatures::brave_vpn_controller() {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  return brave_vpn_controller_.get();
#else
  NOTREACHED();
#endif
}

void BrowserWindowFeatures::Init(BrowserWindowInterface* browser) {
  BrowserWindowFeatures_ChromiumImpl::Init(browser);

  if (brave_rewards::RewardsServiceFactory::GetForProfile(
          browser->GetProfile())) {
    rewards_panel_coordinator_ =
        std::make_unique<brave_rewards::RewardsPanelCoordinator>(browser);
  }
}

void BrowserWindowFeatures::InitPostBrowserViewConstruction(
    BrowserView* browser_view) {
#if !BUILDFLAG(IS_ANDROID)
  if (sidebar::CanUseSidebar(browser_view->browser())) {
    sidebar_controller_ = std::make_unique<sidebar::SidebarController>(
        browser_view->browser(), browser_view->GetProfile());
    if (base::FeatureList::IsEnabled(playlist::features::kPlaylist)) {
      playlist_side_panel_coordinator_ =
          std::make_unique<PlaylistSidePanelCoordinator>(
              browser_view->browser(), sidebar_controller_.get(),
              browser_view->GetProfile());
    }
  }

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  brave_vpn_controller_ = std::make_unique<BraveVPNController>(browser_view);
#endif

  BrowserWindowFeatures_ChromiumImpl::InitPostBrowserViewConstruction(
      browser_view);
#endif  // !BUILDFLAG(IS_ANDROID)
}

void BrowserWindowFeatures::TearDownPreBrowserWindowDestruction() {
  BrowserWindowFeatures_ChromiumImpl::TearDownPreBrowserWindowDestruction();
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  brave_vpn_controller_.reset();
#endif
#if !BUILDFLAG(IS_ANDROID)
  if (sidebar_controller_) {
    sidebar_controller_->TearDownPreBrowserWindowDestruction();
    playlist_side_panel_coordinator_.reset();
  }
#endif  // !BUILDFLAG(IS_ANDROID)
}
