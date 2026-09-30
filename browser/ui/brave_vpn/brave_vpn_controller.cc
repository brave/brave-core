/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_vpn/brave_vpn_controller.h"

#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "url/gurl.h"

DEFINE_USER_DATA(BraveVPNController);

BraveVPNController::BraveVPNController(BrowserView* browser_view)
    : browser_view_(browser_view),
      scoped_unowned_user_data_(
          browser_view->browser()->GetUnownedUserDataHost(),
          *this) {}

BraveVPNController::~BraveVPNController() = default;

// static
BraveVPNController* BraveVPNController::From(BrowserWindowInterface* browser) {
  return Get(browser->GetUnownedUserDataHost());
}

void BraveVPNController::ShowBraveVPNBubble(bool show_select) {
  GetBraveBrowserView()->ShowBraveVPNBubble(show_select);
}

void BraveVPNController::OpenVPNAccountPage(
    brave_vpn::mojom::ManageURLType type) {
  auto* browser = browser_view_->browser();
  auto* profile = browser->GetProfile();
  auto* vpn_service = brave_vpn::BraveVpnServiceFactory::GetForProfile(profile);
  const auto url =
      GURL(brave_vpn::GetManageUrl(vpn_service->GetCurrentEnvironment()));
  ShowSingletonTab(browser, brave_vpn::GetManageURLForUIType(type, url));
}

BraveBrowserView* BraveVPNController::GetBraveBrowserView() {
  return BraveBrowserView::From(browser_view_);
}
