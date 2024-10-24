/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/mac/brave_vpn_service_delegate_mac.h"

#include "brave/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/singleton_tabs.h"

namespace brave_vpn {

BraveVPNServiceDelegateMac::BraveVPNServiceDelegateMac() = default;

BraveVPNServiceDelegateMac::~BraveVPNServiceDelegateMac() = default;

void BraveVPNServiceDelegateMac::WriteConnectionState(
    mojom::ConnectionState state) {}

void BraveVPNServiceDelegateMac::ShowBraveVpnStatusTrayIcon() {}

void BraveVPNServiceDelegateMac::LaunchVPNPanel() {
  auto* browser = chrome::FindBrowserWithActiveWindow();
  if (browser) {
    brave::ShowBraveVPNBubble(browser);
  }
}

void BraveVPNServiceDelegateMac::OpenVpnUI(const std::string& type) {
  // TODO(simonhong): Get proper url based on env.
  GURL checkout_url("https://account.brave.com/?intent=checkout&product=vpn");
  GURL recover_url("https://account.brave.com/?intent=recover&product=vpn");
  auto* browser = chrome::FindBrowserWithActiveWindow();
  if (browser) {
    ShowSingletonTab(browser, type == "checkout" ? checkout_url : recover_url);
  }
}

}  // namespace brave_vpn
