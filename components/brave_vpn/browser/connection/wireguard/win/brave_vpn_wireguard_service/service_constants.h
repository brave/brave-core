/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_SERVICE_SERVICE_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_SERVICE_SERVICE_CONSTANTS_H_

#include <guiddef.h>
#include <string>

namespace brave_vpn {

constexpr wchar_t kBraveWireguardServiceExecutable[] =
    L"brave_vpn_wireguard_service.exe";
constexpr wchar_t kBraveVpnServiceRegistryStoragePath[] =
    L"Software\\BraveSoftware\\Brave\\Vpn\\BraveWireguardService";
constexpr char kBraveWgServiceInstall[] = "install";

const CLSID& GetBraveWireguardServiceClsid();
const IID& GetBraveWireguardServiceIid();
std::wstring GetBraveWireguardServiceName();
std::wstring GetBraveWireguardServiceDisplayName();

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_SERVICE_SERVICE_CONSTANTS_H_
