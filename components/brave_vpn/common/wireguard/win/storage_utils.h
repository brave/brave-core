/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_WIN_STORAGE_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_WIN_STORAGE_UTILS_H_

#include <optional>

#include "base/files/file_path.h"

namespace brave_vpn {

namespace wireguard {
std::wstring GetBraveVpnWireguardServiceRegistryStoragePath();
std::optional<base::FilePath> GetLastUsedConfigPath();
bool UpdateLastUsedConfigPath(const base::FilePath& config_path);
void RemoveStorageKey();
}  // namespace wireguard

bool IsVPNTrayIconEnabled();
void EnableVPNTrayIcon(bool value);

void SetWireguardActive(bool value);
bool IsWireguardActive();

bool ShouldFallbackToIKEv2();
void IncrementWireguardTunnelUsageFlag();
void ResetWireguardTunnelUsageFlag();

void WriteConnectionState(int state);
std::optional<int32_t> GetConnectionState();

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_WIN_STORAGE_UTILS_H_
