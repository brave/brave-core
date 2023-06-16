/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/wireguard_tunnel_service.h"

#include <string>
#include <vector>

#include "base/base64.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/scoped_native_library.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/access_control_list.h"
#include "base/win/registry.h"
#include "base/win/scoped_handle.h"
#include "base/win/security_descriptor.h"
#include "base/win/sid.h"
#include "base/win/windows_types.h"
#include "brave/components/brave_vpn/browser/connection/common/win/scoped_sc_handle.h"
#include "brave/components/brave_vpn/browser/connection/common/win/utils.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/common/service_constants.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/common/wireguard_utils.h"

namespace brave_vpn {

namespace {

constexpr wchar_t kBraveWireguardConfig[] = L"wireguard.brave.conf";

struct SidAccessDescriptor {
  const base::win::Sid& sid;
  DWORD access_mask;
  base::win::SecurityAccessMode access_mode;
};

bool AddACEToPath(const base::FilePath& path,
                  const std::vector<SidAccessDescriptor>& descriptors,
                  DWORD inheritance,
                  bool recursive) {
  DCHECK(!path.empty());
  if (descriptors.empty()) {
    return true;
  }

  // Intentionally take empty descriptor to avoid inherited permissions.
  base::win::SecurityDescriptor sd;

  std::vector<base::win::ExplicitAccessEntry> entries;
  for (const auto& descriptor : descriptors) {
    entries.emplace_back(descriptor.sid, descriptor.access_mode,
                         descriptor.access_mask, inheritance);
  }

  if (!sd.SetDaclEntries(entries)) {
    return false;
  }

  if (recursive) {
    return sd.WriteToFile(path, DACL_SECURITY_INFORMATION);
  }

  base::win::ScopedHandle handle(
      ::CreateFile(path.value().c_str(), WRITE_DAC, 0, nullptr, OPEN_EXISTING,
                   FILE_FLAG_BACKUP_SEMANTICS, nullptr));
  if (!handle.is_valid()) {
    VLOG(1) << "Failed opening path \"" << path.value() << "\" to write DACL";
    return false;
  }
  return sd.WriteToHandle(handle.get(), base::win::SecurityObjectType::kKernel,
                          DACL_SECURITY_INFORMATION);
}

absl::optional<base::FilePath> WriteConfigToFile(const std::string& config) {
  base::FilePath temp_dir_path;
  // Intentionally using base::GetTempDir to reuse same directory between
  // launches.
  if (!base::GetTempDir(&temp_dir_path) || temp_dir_path.empty()) {
    VLOG(1) << "Unable to get temporary directory";
    return absl::nullopt;
  }
  base::ScopedTempDir scoped_temp_dir;
  if (!scoped_temp_dir.Set(temp_dir_path.Append(base::FilePath(L"BraveVpn")))) {
    return absl::nullopt;
  }
  base::FilePath temp_file_path(
      scoped_temp_dir.GetPath().Append(kBraveWireguardConfig));

  if (!base::WriteFile(temp_file_path, config)) {
    VLOG(1) << "Failed to write config to file:" << temp_file_path;
    return absl::nullopt;
  }
  if (!AddACEToPath(
          temp_file_path,
          // Let only windows services to read the config.
          {{base::win::Sid::FromKnownSid(base::win::WellKnownSid::kService),
            GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | DELETE,
            base::win::SecurityAccessMode::kGrant},
           // Let windows administrators only to remove the config.
           {base::win::Sid::FromKnownSid(
                base::win::WellKnownSid::kBuiltinAdministrators),
            GENERIC_EXECUTE | DELETE, base::win::SecurityAccessMode::kGrant}},
          0, /*recursive=*/false)) {
    VLOG(1) << "Failed to set config file permissions:" << temp_file_path;
  }
  // Release temp directory to send path to the WireguardTunnelService.
  scoped_temp_dir.Take();
  return temp_file_path;
}

bool IsServiceRunning(SC_HANDLE service) {
  SERVICE_STATUS service_status = {0};
  if (!::QueryServiceStatus(service, &service_status)) {
    return false;
  }
  return service_status.dwCurrentState == SERVICE_RUNNING;
}

absl::optional<base::FilePath> GetConfigFilePath(
    const std::wstring& encoded_config) {
  if (encoded_config.empty()) {
    return wireguard::GetLastUsedConfigPath();
  }

  std::string decoded_config;
  if (!base::Base64Decode(base::WideToUTF8(encoded_config), &decoded_config) ||
      decoded_config.empty()) {
    VLOG(1) << "Unable to decode wireguard config";
    return absl::nullopt;
  }
  return WriteConfigToFile(decoded_config);
}

bool UpdateLastUsedConfigPath(const base::FilePath& config_path) {
  base::win::RegKey storage;
  if (storage.Create(
          HKEY_LOCAL_MACHINE,
          brave_vpn::GetBraveVpnWireguardServiceRegistryStoragePath().c_str(),
          KEY_SET_VALUE) != ERROR_SUCCESS) {
    return false;
  }
  if (storage.WriteValue(L"ConfigPath", config_path.value().c_str()) !=
      ERROR_SUCCESS) {
    return false;
  }
  return true;
}

}  // namespace

namespace wireguard {

// Creates and launches a new Wireguard Windows service using passed config.
// Before to start a new service it checks and removes existing if exists.
bool LaunchWireguardService(const std::wstring& config) {
  if (!RemoveExistingWireguardService()) {
    VLOG(1) << "Failed to remove existing brave wireguard service";
    return false;
  }
  return CreateAndRunBraveWireguardService(config);
}

bool RemoveExistingWireguardService() {
  ScopedScHandle scm(::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS));
  if (!scm.IsValid()) {
    VLOG(1) << "::OpenSCManager failed. service_name: "
            << GetBraveVpnWireguardTunnelServiceName()
            << ", error: " << std::hex << HRESULTFromLastError();
    return false;
  }
  ScopedScHandle service(
      ::OpenService(scm.Get(), GetBraveVpnWireguardTunnelServiceName().c_str(),
                    SERVICE_ALL_ACCESS));

  if (service.IsValid()) {
    if (IsServiceRunning(service.Get())) {
      SERVICE_STATUS stt;
      if (!ControlService(service.Get(), SERVICE_CONTROL_STOP, &stt)) {
        VLOG(1) << "ControlService failed to send stop signal";
        return false;
      }
      // TODO(spylogsster): Wait until service stopped.
    }
    if (!DeleteService(service.Get())) {
      VLOG(1) << "DeleteService failed, error: " << std::hex
              << HRESULTFromLastError();
      return false;
    }
  }
  return true;
}

// Creates and launches a new Wireguard service with specific config.
bool CreateAndRunBraveWireguardService(const std::wstring& encoded_config) {
  auto config_file_path = GetConfigFilePath(encoded_config);
  if (!config_file_path.has_value()) {
    VLOG(1) << "Unable to get wireguard config";
    return false;
  }
  ScopedScHandle scm(::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS));
  if (!scm.IsValid()) {
    VLOG(1) << "::OpenSCManager failed. service_name: "
            << GetBraveVpnWireguardTunnelServiceName()
            << ", error: " << std::hex << HRESULTFromLastError();
    return false;
  }

  base::FilePath directory;
  if (!base::PathService::Get(base::DIR_EXE, &directory)) {
    return false;
  }
  base::CommandLine service_cmd(
      directory.Append(brave_vpn::kBraveVpnWireguardServiceExecutable));
  service_cmd.AppendSwitchPath(
      brave_vpn::kBraveVpnWireguardServiceConnectSwitchName,
      config_file_path.value());

  ScopedScHandle service(::CreateService(
      scm.Get(), GetBraveVpnWireguardTunnelServiceName().c_str(),
      GetBraveVpnWireguardTunnelServiceName().c_str(), SERVICE_ALL_ACCESS,
      SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
      service_cmd.GetCommandLineString().c_str(), NULL, NULL, L"Nsi\0TcpIp\0",
      NULL, NULL));
  if (!service.IsValid()) {
    VLOG(1) << "::CreateService failed. service_name: "
            << GetBraveVpnWireguardTunnelServiceName() << ", error: 0x"
            << ::GetLastError();
    return false;
  }

  SERVICE_SID_INFO info;
  info.dwServiceSidType = SERVICE_SID_TYPE_UNRESTRICTED;
  if (!ChangeServiceConfig2(service.Get(), SERVICE_CONFIG_SERVICE_SID_INFO,
                            &info)) {
    VLOG(1) << "Failed to configure service 0x" << std::hex
            << HRESULTFromLastError();
    return false;
  }

  if (!brave_vpn::SetServiceFailureActions(service.Get())) {
    VLOG(1) << "SetServiceFailActions failed:" << std::hex
            << HRESULTFromLastError();
    return false;
  }

  if (!StartService(service.Get(), 0, NULL)) {
    VLOG(1) << "Failed to start service 0x" << std::hex
            << HRESULTFromLastError();
    return false;
  }
  if (!encoded_config.empty() &&
      !UpdateLastUsedConfigPath(config_file_path.value())) {
    VLOG(1) << "Failed to save last used config path";
  }

  return DeleteService(service.Get()) != 0;
}

int RunWireguardTunnelService(const base::FilePath& config_file_path) {
  if (config_file_path.empty()) {
    VLOG(1) << "Wrong path to config file:" << config_file_path;
    return S_FALSE;
  }

  {
    base::FilePath directory;
    if (!base::PathService::Get(base::DIR_EXE, &directory)) {
      return S_FALSE;
    }
    typedef bool WireGuardTunnelService(const LPCWSTR settings);
    base::ScopedNativeLibrary tunnel_lib(directory.Append(L"tunnel.dll"));

    WireGuardTunnelService* tunnel_proc =
        reinterpret_cast<WireGuardTunnelService*>(
            tunnel_lib.GetFunctionPointer("WireGuardTunnelService"));
    if (!tunnel_proc) {
      VLOG(1) << __func__ << ": WireGuardTunnelService not found error: "
              << tunnel_lib.GetError()->ToString();
      return S_FALSE;
    }

    if (tunnel_proc(config_file_path.value().c_str())) {
      return S_OK;
    }
    VLOG(1) << "Failed to activate tunnel service:"
            << tunnel_lib.GetError()->ToString();
  }
  return S_FALSE;
}

bool WireguardGenerateKeypair(std::string* public_key,
                              std::string* private_key) {
  base::FilePath directory;
  if (!base::PathService::Get(base::DIR_EXE, &directory)) {
    VLOG(1) << __func__ << ": executable path not found";
    return false;
  }
  base::ScopedNativeLibrary tunnel_lib(directory.Append(L"tunnel.dll"));
  typedef bool WireGuardGenerateKeypair(uint8_t[32], uint8_t[32]);
  std::vector<uint8_t> public_key_bytes(32);
  std::vector<uint8_t> private_key_bytes(32);

  WireGuardGenerateKeypair* generate_proc =
      reinterpret_cast<WireGuardGenerateKeypair*>(
          tunnel_lib.GetFunctionPointer("WireGuardGenerateKeypair"));
  if (!generate_proc) {
    VLOG(1) << __func__ << ": WireGuardGenerateKeypair not found error: "
            << tunnel_lib.GetError()->ToString();
    return false;
  }
  if (generate_proc(public_key_bytes.data(), private_key_bytes.data())) {
    VLOG(1) << __func__ << "Unable to generate keys, error:"
            << tunnel_lib.GetError()->ToString();
    return false;
  }

  *public_key = base::Base64Encode(base::span<const uint8_t>(public_key_bytes));
  *private_key =
      base::Base64Encode(base::span<const uint8_t>(private_key_bytes));
  return true;
}

}  // namespace wireguard
}  // namespace brave_vpn
