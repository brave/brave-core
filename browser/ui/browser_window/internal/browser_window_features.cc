/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/browser_window/public/browser_window_features.h"

#include "base/memory/ptr_util.h"
#include "base/notreached.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/email_aliases/email_aliases_service_factory.h"
#include "brave/browser/ui/brave_browser_window.h"
#include "brave/browser/ui/brave_rewards/rewards_panel_coordinator.h"
#include "brave/browser/ui/email_aliases/email_aliases_controller.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/views/page_info/brave_shields_ui_contents_cache.h"
#include "brave/browser/ui/views/side_panel/playlist/playlist_side_panel_coordinator.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/playlist/core/common/features.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/ui/brave_vpn/brave_vpn_controller.h"
#endif

#if !BUILDFLAG(ENABLE_BRAVE_VPN)
// Use stub class to avoid incomplete type build error.
class BraveVPNController {};
#endif

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

  brave_shields_ui_contents_cache_ =
      std::make_unique<BraveShieldsUIContentsCache>();
}

void BrowserWindowFeatures::InitPostBrowserViewConstruction(
    BrowserView* browser_view) {
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

  if (auto* email_aliases_service =
          email_aliases::EmailAliasesServiceFactory::GetServiceForProfile(
              browser_view->GetProfile())) {
    email_aliases_controller_ =
        std::make_unique<email_aliases::EmailAliasesController>(
            browser_view, email_aliases_service);
  }

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  brave_vpn_controller_ = std::make_unique<BraveVPNController>(browser_view);
#endif

  BrowserWindowFeatures_ChromiumImpl::InitPostBrowserViewConstruction(
      browser_view);
}

void BrowserWindowFeatures::TearDownPreBrowserWindowDestruction() {
  BrowserWindowFeatures_ChromiumImpl::TearDownPreBrowserWindowDestruction();
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  brave_vpn_controller_.reset();
#endif

  email_aliases_controller_.reset();

  if (sidebar_controller_) {
    sidebar_controller_->TearDownPreBrowserWindowDestruction();
    playlist_side_panel_coordinator_.reset();
  }
}
