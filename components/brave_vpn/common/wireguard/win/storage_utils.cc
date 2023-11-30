/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/common/wireguard/win/storage_utils.h"

#include <optional>

#include "base/logging.h"
#include "base/win/registry.h"
#include "brave/components/brave_vpn/common/wireguard/win/service_constants.h"
#include "brave/components/brave_vpn/common/wireguard/win/service_details.h"
#include "brave/components/brave_vpn/common/wireguard/win/wireguard_utils_win.h"

namespace brave_vpn {

namespace {
// Registry path to Wireguard vpn service storage.
constexpr wchar_t kBraveVpnWireguardServiceRegistryStoragePath[] =
    L"Software\\BraveSoftware\\Vpn\\";
constexpr wchar_t kBraveWireguardConfigKeyName[] = L"ConfigPath";
constexpr wchar_t kBraveWireguardEnableTrayIconKeyName[] = L"EnableTrayIcon";
constexpr wchar_t kBraveWireguardActiveKeyName[] = L"WireGuardActive";
constexpr wchar_t kBraveWireguardConnectionStateName[] = L"ConnectionState";
constexpr uint16_t kBraveVpnWireguardMaxFailedAttempts = 3;

std::optional<base::win::RegKey> GetStorageKey(HKEY root_key, REGSAM access) {
  base::win::RegKey storage;
  if (storage.Create(
          root_key,
          brave_vpn::wireguard::GetBraveVpnWireguardServiceRegistryStoragePath()
              .c_str(),
          access) != ERROR_SUCCESS) {
    return std::nullopt;
  }

  return storage;
}

}  // namespace

namespace wireguard {

std::wstring GetBraveVpnWireguardServiceRegistryStoragePath() {
  return kBraveVpnWireguardServiceRegistryStoragePath +
         brave_vpn::GetBraveVpnWireguardServiceName();
}

// Returns last used config path.
// We keep config file between launches to be able to reuse it outside of Brave.
std::optional<base::FilePath> GetLastUsedConfigPath() {
  auto storage = GetStorageKey(HKEY_LOCAL_MACHINE, KEY_QUERY_VALUE);
  if (!storage.has_value()) {
    return std::nullopt;
  }

  std::wstring value;
  if (storage->ReadValue(kBraveWireguardConfigKeyName, &value) !=
          ERROR_SUCCESS ||
      value.empty()) {
    return std::nullopt;
  }
  return base::FilePath(value);
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

void RemoveStorageKey() {
  if (base::win::RegKey(HKEY_LOCAL_MACHINE,
                        kBraveVpnWireguardServiceRegistryStoragePath,
                        KEY_ALL_ACCESS)
          .DeleteKey(brave_vpn::GetBraveVpnWireguardServiceName().c_str()) !=
      ERROR_SUCCESS) {
    VLOG(1) << "Failed to delete storage registry value";
  }
}

}  // namespace wireguard

bool IsVPNTrayIconEnabled() {
  auto storage = GetStorageKey(HKEY_CURRENT_USER, KEY_QUERY_VALUE);
  if (!storage.has_value()) {
    return true;
  }

  DWORD value = 1;
  if (storage->ReadValueDW(kBraveWireguardEnableTrayIconKeyName, &value) !=
      ERROR_SUCCESS) {
    return true;
  }
  return value == 1;
}

void EnableVPNTrayIcon(bool value) {
  auto storage = GetStorageKey(HKEY_CURRENT_USER, KEY_SET_VALUE);
  if (!storage.has_value()) {
    return;
  }

  if (storage->WriteValue(kBraveWireguardEnableTrayIconKeyName, DWORD(value)) !=
      ERROR_SUCCESS) {
    VLOG(1) << "False to write registry value";
  }
}

void SetWireguardActive(bool value) {
  auto storage = GetStorageKey(HKEY_CURRENT_USER, KEY_SET_VALUE);
  if (!storage.has_value()) {
    return;
  }

  if (storage->WriteValue(kBraveWireguardActiveKeyName, DWORD(value)) !=
      ERROR_SUCCESS) {
    VLOG(1) << "False to write registry value";
  }
}

bool IsWireguardActive() {
  auto storage = GetStorageKey(HKEY_CURRENT_USER, KEY_QUERY_VALUE);
  if (!storage.has_value()) {
    return true;
  }

  DWORD value = 1;
  if (storage->ReadValueDW(kBraveWireguardActiveKeyName, &value) !=
      ERROR_SUCCESS) {
    return true;
  }
  return value == 1;
}

// If the tunnel service failed to launch or crashed more than the limit we
// should ask user for the fallback to IKEv2 implementation.
bool ShouldFallbackToIKEv2() {
  auto storage = GetStorageKey(HKEY_LOCAL_MACHINE, KEY_READ);
  if (!storage.has_value()) {
    return true;
  }

  DWORD launch = 0;
  storage->ReadValueDW(kBraveVpnWireguardCounterOfTunnelUsage, &launch);
  return launch >= kBraveVpnWireguardMaxFailedAttempts ||
         !wireguard::IsWireguardServiceRegistered();
}

// Increments number of usages for the wireguard tunnel service.
void IncrementWireguardTunnelUsageFlag() {
  base::win::RegKey key(
      HKEY_LOCAL_MACHINE,
      wireguard::GetBraveVpnWireguardServiceRegistryStoragePath().c_str(),
      KEY_ALL_ACCESS);
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
      wireguard::GetBraveVpnWireguardServiceRegistryStoragePath().c_str(),
      KEY_ALL_ACCESS);
  if (!key.Valid()) {
    VLOG(1) << "Failed to open vpn service storage";
    return;
  }
  key.DeleteValue(kBraveVpnWireguardCounterOfTunnelUsage);
}

void WriteConnectionState(int value) {
  auto storage = GetStorageKey(HKEY_CURRENT_USER, KEY_SET_VALUE);
  if (!storage.has_value()) {
    return;
  }
  if (storage->WriteValue(kBraveWireguardConnectionStateName, DWORD(value)) !=
      ERROR_SUCCESS) {
    VLOG(1) << "False to write registry value";
  }
}

std::optional<int32_t> GetConnectionState() {
  auto storage = GetStorageKey(HKEY_CURRENT_USER, KEY_QUERY_VALUE);
  if (!storage.has_value()) {
    return std::nullopt;
  }
  DWORD value;
  if (storage->ReadValueDW(kBraveWireguardConnectionStateName, &value) ==
      ERROR_SUCCESS) {
    return value;
  }
  return std::nullopt;
}

}  // namespace brave_vpn
