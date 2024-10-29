/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_service_delegate_win.h"

#include "brave/browser/brave_vpn/win/storage_utils.h"
#include "brave/browser/brave_vpn/win/wireguard_utils_win.h"

namespace brave_vpn {

BraveVPNServiceDelegateWin::BraveVPNServiceDelegateWin() = default;

BraveVPNServiceDelegateWin::~BraveVPNServiceDelegateWin() = default;

void BraveVPNServiceDelegateWin::WriteConnectionState(
    mojom::ConnectionState state) {
  ::brave_vpn::WriteConnectionState(static_cast<int>(state));
}

void BraveVPNServiceDelegateWin::ShowBraveVpnStatusTrayIcon() {
  wireguard::ShowBraveVpnStatusTrayIcon();
}

}  // namespace brave_vpn
