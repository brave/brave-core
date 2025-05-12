/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/browser_window/public/browser_window_features.h"

#include "base/memory/ptr_util.h"
#include "base/notreached.h"
#include "brave/browser/ui/brave_browser_window.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/split_view/split_view_controller.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/sidebar/common/features.h"
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

BrowserWindowFeatures::~BrowserWindowFeatures() {
  if (sidebar_controller_) {
    sidebar_controller_->set_web_panel_delegate(nullptr);
  }
}

BraveVPNController* BrowserWindowFeatures::brave_vpn_controller() {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  return brave_vpn_controller_.get();
#else
  NOTREACHED();
#endif
}

SplitViewBrowserData* BrowserWindowFeatures::split_view_browser_data() {
  return split_view_controller_
             ? split_view_controller_->split_view_browser_data()
             : nullptr;
}

void BrowserWindowFeatures::Init(
    BrowserWindowInterface* browser_window_interface) {
  BrowserWindowFeatures_ChromiumImpl::Init(browser_window_interface);

  if (browser_window_interface->GetType() ==
          BrowserWindowInterface::TYPE_NORMAL &&
      base::FeatureList::IsEnabled(tabs::features::kBraveSplitView)) {
    split_view_controller_ = std::make_unique<SplitViewController>(
        browser_window_interface->GetTabStripModel());
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

  const bool is_type_normal =
      browser->GetType() == BrowserWindowInterface::TYPE_NORMAL;
  if (is_type_normal) {
    sidebar_controller_ = std::make_unique<sidebar::SidebarController>(browser);
    sidebar_controller_->set_side_panel_ui(side_panel_ui());
    static_cast<BraveBrowserWindow*>(browser->window())->InitSidebar();

    if (split_view_controller_ &&
        base::FeatureList::IsEnabled(sidebar::features::kSidebarWebPanel)) {
      auto* web_panel_data =
          split_view_controller_->split_view_web_panel_data();
      web_panel_data->SetBrowser(browser);
      sidebar_controller_->set_web_panel_delegate(web_panel_data);
    }
  }
}

void BrowserWindowFeatures::TearDownPreBrowserViewDestruction() {
  // As |sidebar_controller_| depends on side_panel_ui(),
  // reset it before panel ui is gone.
  if (sidebar_controller_) {
    sidebar_controller_->set_side_panel_ui(nullptr);
  }
  BrowserWindowFeatures_ChromiumImpl::TearDownPreBrowserViewDestruction();
}
