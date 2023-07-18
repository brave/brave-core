/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_SERVICE_TUNNEL_UTILS_H_
#define BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_SERVICE_TUNNEL_UTILS_H_

#include "base/files/file_path.h"

namespace brave_vpn {

namespace wireguard {

void IncrementWireguardTunnelUsageFlag();
void ResetWireguardTunnelUsageFlag();
bool UpdateLastUsedConfigPath(const base::FilePath& config_path);

}  // namespace wireguard

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_SERVICE_TUNNEL_UTILS_H_
