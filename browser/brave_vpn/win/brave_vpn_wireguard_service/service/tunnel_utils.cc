/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/service/tunnel_utils.h"

#include "base/logging.h"
#include "base/win/registry.h"
#include "brave/components/brave_vpn/common/wireguard/win/service_constants.h"
#include "brave/components/brave_vpn/common/wireguard/win/storage_utils.h"

namespace brave_vpn {

namespace wireguard {

namespace {
constexpr wchar_t kBraveWireguardConfigKeyName[] = L"ConfigPath";
}  // namespace

// Increments number of usages for the wireguard tunnel service.
void IncrementWireguardTunnelUsageFlag() {
  base::win::RegKey key(
      HKEY_LOCAL_MACHINE,
      GetBraveVpnWireguardServiceRegistryStoragePath().c_str(), KEY_ALL_ACCESS);
  if (!key.Valid()) {
    VLOG(1) << "Failed to open wireguard service storage";
    return;
  }
  DWORD launch = 0;
  key.ReadValueDW(kBraveVpnWireguardCounterOfTunnelUsage, &launch);
  launch++;
  key.WriteValue(kBraveVpnWireguardCounterOfTunnelUsage, launch);
}

// Resets number of launches for the wireguard tunnel service.
void ResetWireguardTunnelUsageFlag() {
  base::win::RegKey key(
      HKEY_LOCAL_MACHINE,
      GetBraveVpnWireguardServiceRegistryStoragePath().c_str(), KEY_ALL_ACCESS);
  if (!key.Valid()) {
    VLOG(1) << "Failed to open vpn service storage";
    return;
  }
  key.DeleteValue(kBraveVpnWireguardCounterOfTunnelUsage);
}

bool UpdateLastUsedConfigPath(const base::FilePath& config_path) {
  base::win::RegKey storage;
  if (storage.Create(HKEY_LOCAL_MACHINE,
                     GetBraveVpnWireguardServiceRegistryStoragePath().c_str(),
                     KEY_SET_VALUE) != ERROR_SUCCESS) {
    return false;
  }
  if (storage.WriteValue(kBraveWireguardConfigKeyName,
                         config_path.value().c_str()) != ERROR_SUCCESS) {
    return false;
  }
  return true;
}

}  // namespace wireguard
}  // namespace brave_vpn
