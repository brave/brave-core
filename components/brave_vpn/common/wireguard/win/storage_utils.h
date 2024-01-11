/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_WIN_STORAGE_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_WIN_STORAGE_UTILS_H_

#include <optional>

#include "base/files/file_path.h"
#include "components/version_info/channel.h"

namespace brave_vpn {

namespace wireguard {
std::wstring GetBraveVpnWireguardServiceRegistryStoragePath(
    version_info::Channel channel);
std::optional<base::FilePath> GetLastUsedConfigPath(
    version_info::Channel channel);
bool UpdateLastUsedConfigPath(const base::FilePath& config_path,
                              version_info::Channel channel);
void RemoveStorageKey(version_info::Channel channel);
}  // namespace wireguard

bool IsVPNTrayIconEnabled(version_info::Channel channel);
void EnableVPNTrayIcon(bool value, version_info::Channel channel);

void SetWireguardActive(bool value, version_info::Channel channel);
bool IsWireguardActive(version_info::Channel channel);

bool ShouldFallbackToIKEv2(version_info::Channel channel);
void IncrementWireguardTunnelUsageFlag(version_info::Channel channel);
void ResetWireguardTunnelUsageFlag(version_info::Channel channel);

void WriteConnectionState(int state, version_info::Channel channel);
std::optional<int32_t> GetConnectionState(version_info::Channel channel);

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_WIN_STORAGE_UTILS_H_
