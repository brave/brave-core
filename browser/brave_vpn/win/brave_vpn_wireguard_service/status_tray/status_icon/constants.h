/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_STATUS_ICON_CONSTANTS_H_
#define BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_STATUS_ICON_CONSTANTS_H_

#include "base/files/file_path.h"

namespace brave_vpn {

// Status tray icon window name and class.
inline constexpr base::FilePath::CharType kStatusTrayWindowName[] =
    FILE_PATH_LITERAL("BraveVpn_StatusTrayWindow");
inline constexpr base::FilePath::CharType kStatusTrayWindowClass[] =
    FILE_PATH_LITERAL("BraveVpn_StatusTraydowClass");
inline constexpr base::FilePath::CharType kBraveVpnStatusTrayMessageName[] =
    FILE_PATH_LITERAL("BraveVpn_CustomTrayMessage");
}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_STATUS_ICON_CONSTANTS_H_
