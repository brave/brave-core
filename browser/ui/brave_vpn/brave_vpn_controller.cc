/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_vpn/brave_vpn_controller.h"

#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "url/gurl.h"

BraveVPNController::BraveVPNController(BrowserView* browser_view)
    : browser_view_(browser_view) {}

BraveVPNController::~BraveVPNController() = default;

void BraveVPNController::ShowBraveVPNBubble(bool show_select) {
  GetBraveBrowserView()->ShowBraveVPNBubble(show_select);
}

void BraveVPNController::OpenVPNAccountPage() {
  auto* browser = browser_view_->browser();
  auto* profile = browser->profile();
  auto* vpn_service = brave_vpn::BraveVpnServiceFactory::GetForProfile(profile);
  const auto url =
      brave_vpn::GetManageUrl(vpn_service->GetCurrentEnvironment());
  ShowSingletonTab(browser, GURL(url));
}

BraveBrowserView* BraveVPNController::GetBraveBrowserView() {
  return static_cast<BraveBrowserView*>(browser_view_);
}
