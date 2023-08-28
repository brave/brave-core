/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_WIN_STORAGE_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_WIN_STORAGE_UTILS_H_

#include "base/files/file_path.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_vpn {

namespace wireguard {

std::wstring GetBraveVpnWireguardServiceRegistryStoragePath();

bool IsVPNTrayIconEnabled();
void EnableVPNTrayIcon(bool value);

void SetWireguardActive(bool value);
bool IsWireguardActive();

absl::optional<base::FilePath> GetLastUsedConfigPath();
bool UpdateLastUsedConfigPath(const base::FilePath& config_path);

bool ShouldFallbackToIKEv2();
void IncrementWireguardTunnelUsageFlag();
void ResetWireguardTunnelUsageFlag();

void RemoveStorageKey();

void WriteConnectionState(int state);
absl::optional<int32_t> GetConnectionState();
}  // namespace wireguard
}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_WIN_STORAGE_UTILS_H_
