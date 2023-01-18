/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_BRAVE_VPN_HELPER_BRAVE_VPN_HELPER_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_BRAVE_VPN_HELPER_BRAVE_VPN_HELPER_CONSTANTS_H_

namespace brave_vpn {

constexpr char kBraveVPNHelperProcessType[] = "brave-vpn-helper";
constexpr char kBraveVpnHelperInstall[] = "install";
constexpr wchar_t kBraveVpnHelperRegistryStoragePath[] =
    L"Software\\BraveSoftware\\Brave\\Vpn\\HelperService";
constexpr wchar_t kBraveVPNHelperExecutable[] = L"brave_vpn_helper.exe";
constexpr wchar_t kBraveVpnServiceName[] = L"BraveVPNService";
constexpr wchar_t kBraveVpnHelperLaunchCounterValue[] = L"launched";
}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_BRAVE_VPN_HELPER_BRAVE_VPN_HELPER_CONSTANTS_H_
