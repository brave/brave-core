/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/browser_window/public/browser_window_features.h"

#include "base/memory/ptr_util.h"
#include "base/notreached.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/ui/brave_browser_window.h"
#include "brave/browser/ui/brave_rewards/rewards_panel_coordinator.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/ui/brave_vpn/brave_vpn_controller.h"
#endif

#if !BUILDFLAG(ENABLE_BRAVE_VPN)
// Use stub class to avoid incomplete type build error.
class BraveVPNController {};
#endif

// static
std::unique_ptr<BrowserWindowFeatures>
BrowserWindowFeatures::CreateBrowserWindowFeatures() {
  return base::WrapUnique(new BrowserWindowFeatures());
}

// static
void BrowserWindowFeatures::ReplaceBrowserWindowFeaturesForTesting(
    BrowserWindowFeaturesFactory factory) {
  // Upstream doesn't use this static method.
  // Revisit if used. Need to handle GetFactory() overriding.
  NOTREACHED();
}

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

  if (tabs::features::IsBraveSplitViewEnabled()) {
    split_view_browser_data_ = std::make_unique<SplitViewBrowserData>(browser);
  }

  if (brave_rewards::RewardsServiceFactory::GetForProfile(
          browser->GetProfile())) {
    rewards_panel_coordinator_ =
        std::make_unique<brave_rewards::RewardsPanelCoordinator>(browser);
  }
}

void BrowserWindowFeatures::InitPostBrowserViewConstruction(
    BrowserView* browser_view) {
  BrowserWindowFeatures_ChromiumImpl::InitPostBrowserViewConstruction(
      browser_view);

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  brave_vpn_controller_ = std::make_unique<BraveVPNController>(browser_view);
#endif
}

void BrowserWindowFeatures::InitPostWindowConstruction(Browser* browser) {
  BrowserWindowFeatures_ChromiumImpl::InitPostWindowConstruction(browser);

  if (sidebar::CanUseSidebar(browser)) {
    sidebar_controller_ = std::make_unique<sidebar::SidebarController>(
        browser, browser->profile());
  }
}
