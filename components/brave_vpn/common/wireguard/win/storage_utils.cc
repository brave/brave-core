/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/common/wireguard/win/storage_utils.h"

#include "base/logging.h"
#include "base/win/registry.h"
#include "brave/components/brave_vpn/common/wireguard/win/service_constants.h"
#include "brave/components/brave_vpn/common/wireguard/win/service_details.h"

namespace brave_vpn {

namespace wireguard {

namespace {
// Registry path to Wireguard vpn service storage.
constexpr wchar_t kBraveVpnWireguardServiceRegistryStoragePath[] =
    L"Software\\BraveSoftware\\Vpn\\";
constexpr wchar_t kBraveWireguardConfigKeyName[] = L"ConfigPath";
constexpr wchar_t kBraveWireguardEnableTrayIconKeyName[] = L"EnableTrayIcon";
constexpr uint16_t kBraveVpnWireguardMaxFailedAttempts = 3;
}  // namespace

std::wstring GetBraveVpnWireguardServiceRegistryStoragePath() {
  return kBraveVpnWireguardServiceRegistryStoragePath +
         brave_vpn::GetBraveVpnWireguardServiceName();
}

bool IsVPNTrayIconEnabled() {
  base::win::RegKey storage;
  if (storage.Open(HKEY_CURRENT_USER,
                   GetBraveVpnWireguardServiceRegistryStoragePath().c_str(),
                   KEY_QUERY_VALUE) != ERROR_SUCCESS) {
    return true;
  }
  DWORD value = 1;
  if (storage.ReadValueDW(kBraveWireguardEnableTrayIconKeyName, &value) !=
      ERROR_SUCCESS) {
    return true;
  }
  return value == 1;
}

void EnableVPNTrayIcon(bool value) {
  base::win::RegKey storage;
  if (storage.Create(HKEY_CURRENT_USER,
                     GetBraveVpnWireguardServiceRegistryStoragePath().c_str(),
                     KEY_SET_VALUE) != ERROR_SUCCESS) {
    return;
  }

  if (storage.WriteValue(kBraveWireguardEnableTrayIconKeyName, DWORD(value)) !=
      ERROR_SUCCESS) {
    VLOG(1) << "False to write registry value";
  }
}

// If the tunnel service failed to launch or crashed more than the limit we
// should ask user for the fallback to IKEv2 implementation.
bool ShouldFallbackToIKEv2() {
  base::win::RegKey key(
      HKEY_LOCAL_MACHINE,
      GetBraveVpnWireguardServiceRegistryStoragePath().c_str(), KEY_READ);
  if (!key.Valid()) {
    VLOG(1) << "Failed to open wireguard service storage";
    return false;
  }
  DWORD launch = 0;
  key.ReadValueDW(kBraveVpnWireguardCounterOfTunnelUsage, &launch);
  return launch >= kBraveVpnWireguardMaxFailedAttempts;
}

// Returns last used config path.
// We keep config file between launches to be able to reuse it outside of Brave.
absl::optional<base::FilePath> GetLastUsedConfigPath() {
  base::win::RegKey storage;
  if (storage.Open(HKEY_LOCAL_MACHINE,
                   GetBraveVpnWireguardServiceRegistryStoragePath().c_str(),
                   KEY_QUERY_VALUE) != ERROR_SUCCESS) {
    return absl::nullopt;
  }
  std::wstring value;
  if (storage.ReadValue(kBraveWireguardConfigKeyName, &value) !=
          ERROR_SUCCESS ||
      value.empty()) {
    return absl::nullopt;
  }
  return base::FilePath(value);
}

}  // namespace wireguard
}  // namespace brave_vpn
