/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_STATUS_ICON_ICON_UTILS_H_
#define BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_STATUS_ICON_ICON_UTILS_H_

#include "base/win/windows_types.h"

namespace gfx {
class ImageSkia;
class Size;
}  // namespace gfx

namespace brave_vpn {
gfx::ImageSkia GetIconFromResources(int icon_id, const gfx::Size& size);
bool IsBraveVpnTrayIconRunning();
HWND GetBraveVpnStatusTrayIconHWND();
}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_STATUS_ICON_ICON_UTILS_H_
