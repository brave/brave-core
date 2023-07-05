/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/service/tunnel_utils.h"

#include "base/logging.h"
#include "base/win/registry.h"
#include "brave/components/brave_vpn/common/wireguard/win/service_constants.h"

namespace brave_vpn {

namespace wireguard {

namespace {
constexpr wchar_t kBraveWireguardConfigKeyName[] = L"ConfigPath";
}  // namespace

// Increments number of launches for the wireguard tunnel service.
void IncrementWireguardTunnelLaunchedFlag() {
  base::win::RegKey key(
      HKEY_LOCAL_MACHINE,
      brave_vpn::GetBraveVpnWireguardServiceRegistryStoragePath().c_str(),
      KEY_ALL_ACCESS);
  if (!key.Valid()) {
    VLOG(1) << "Failed to open wireguard service storage";
    return;
  }
  DWORD launch = 0;
  key.ReadValueDW(kBraveVpnWireguardCounterOfTunnelLaunches, &launch);
  launch++;
  key.WriteValue(kBraveVpnWireguardCounterOfTunnelLaunches, launch);
}

// Resets number of launches for the wireguard tunnel service.
void ResetWireguardTunnelLaunchedFlag() {
  base::win::RegKey key(
      HKEY_LOCAL_MACHINE,
      brave_vpn::GetBraveVpnWireguardServiceRegistryStoragePath().c_str(),
      KEY_ALL_ACCESS);
  if (!key.Valid()) {
    VLOG(1) << "Failed to open vpn service storage";
    return;
  }
  key.DeleteValue(kBraveVpnWireguardCounterOfTunnelLaunches);
}

bool UpdateLastUsedConfigPath(const base::FilePath& config_path) {
  base::win::RegKey storage;
  if (storage.Create(
          HKEY_LOCAL_MACHINE,
          brave_vpn::GetBraveVpnWireguardServiceRegistryStoragePath().c_str(),
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
