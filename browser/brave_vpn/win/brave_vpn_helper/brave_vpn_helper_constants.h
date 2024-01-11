/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_HELPER_BRAVE_VPN_HELPER_CONSTANTS_H_
#define BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_HELPER_BRAVE_VPN_HELPER_CONSTANTS_H_

#include <guiddef.h>

namespace brave_vpn {

inline constexpr char kBraveVpnHelperCrashMe[] = "crash-me";
inline constexpr wchar_t kBraveVPNHelperExecutable[] = L"brave_vpn_helper.exe";
inline constexpr wchar_t kBraveVpnHelperFiltersInstalledValue[] = L"filters";
inline constexpr wchar_t kBraveVpnOneTimeServiceCleanupValue[] = L"ran";

// Repeating interval to check the connection is live.
inline constexpr int kCheckConnectionIntervalInSeconds = 3;

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_HELPER_BRAVE_VPN_HELPER_CONSTANTS_H_
