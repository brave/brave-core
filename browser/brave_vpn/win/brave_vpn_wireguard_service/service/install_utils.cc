/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/service/install_utils.h"

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/win/registry.h"
#include "base/win/windows_types.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/service/wireguard_tunnel_service.h"
#include "brave/components/brave_vpn/common/win/scoped_sc_handle.h"
#include "brave/components/brave_vpn/common/win/utils.h"
#include "brave/components/brave_vpn/common/wireguard/win/service_constants.h"
#include "brave/components/brave_vpn/common/wireguard/win/service_details.h"
#include "brave/components/brave_vpn/common/wireguard/win/storage_utils.h"
#include "brave/components/brave_vpn/common/wireguard/win/wireguard_utils_win.h"
#include "chrome/installer/util/install_service_work_item.h"

namespace brave_vpn {

namespace {

constexpr wchar_t kAutoRunKeyPath[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

bool RemoveWireguardConfigDirectory(const base::FilePath& last_used_config) {
  auto wireguard_config_folder = last_used_config.DirName();
  if (wireguard_config_folder.empty() ||
      !base::PathExists(wireguard_config_folder)) {
    return true;
  }

  return base::DeletePathRecursively(wireguard_config_folder);
}

void AddToStartup(const std::wstring& value,
                  const base::CommandLine& command_line) {
  base::win::RegKey key(HKEY_LOCAL_MACHINE, kAutoRunKeyPath, KEY_WRITE);
  if (!key.Valid()) {
    VLOG(1) << "Failed to write wireguard service to startup";
    return;
  }
  key.WriteValue(value.c_str(), command_line.GetCommandLineString().c_str());
}

void RemoveFromStartup(const std::wstring& value) {
  base::win::RegKey key(HKEY_LOCAL_MACHINE, kAutoRunKeyPath, KEY_WRITE);
  if (!key.Valid()) {
    VLOG(1) << "Failed to write wireguard service to startup";
    return;
  }
  key.DeleteValue(value.c_str());
}
}  // namespace

bool ConfigureBraveWireguardService(const std::wstring& service_name) {
  ScopedScHandle scm(::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS));
  if (!scm.IsValid()) {
    VLOG(1) << "::OpenSCManager failed. service_name: " << service_name
            << ", error: " << std::hex << HRESULTFromLastError();
    return false;
  }
  base::FilePath exe_path;
  if (!base::PathService::Get(base::FILE_EXE, &exe_path)) {
    return S_OK;
  }

  ScopedScHandle service(
      ::OpenService(scm.Get(), service_name.c_str(), SERVICE_ALL_ACCESS));
  if (!service.IsValid()) {
    VLOG(1) << "Failed to create service_name: " << service_name
            << ", error: " << std::hex << HRESULTFromLastError();
    return false;
  }
  SERVICE_SID_INFO info = {};
  info.dwServiceSidType = SERVICE_SID_TYPE_UNRESTRICTED;
  if (!ChangeServiceConfig2(service.Get(), SERVICE_CONFIG_SERVICE_SID_INFO,
                            &info)) {
    VLOG(1) << "ChangeServiceConfig2 failed:" << std::hex
            << HRESULTFromLastError();
    return false;
  }
  return true;
}

// Installs Brave VPN Wireguard Windows service and configures the service
// config.
bool InstallBraveWireguardService() {
  base::FilePath exe_dir;
  base::PathService::Get(base::DIR_EXE, &exe_dir);
  base::CommandLine service_cmd(
      exe_dir.Append(brave_vpn::kBraveVpnWireguardServiceExecutable));
  installer::InstallServiceWorkItem install_service_work_item(
      brave_vpn::GetBraveVpnWireguardServiceName(),
      brave_vpn::GetBraveVpnWireguardServiceDisplayName(), SERVICE_DEMAND_START,
      service_cmd, base::CommandLine(base::CommandLine::NO_PROGRAM),
      brave_vpn::wireguard::GetBraveVpnWireguardServiceRegistryStoragePath(),
      {brave_vpn::GetBraveVpnWireguardServiceClsid()},
      {brave_vpn::GetBraveVpnWireguardServiceIid()});
  install_service_work_item.set_best_effort(true);
  install_service_work_item.set_rollback_enabled(false);
  if (install_service_work_item.Do()) {
    auto success = brave_vpn::ConfigureBraveWireguardService(
        brave_vpn::GetBraveVpnWireguardServiceName());
    if (success) {
      service_cmd.AppendSwitch(
          brave_vpn::kBraveVpnWireguardServiceInteractiveSwitchName);
      AddToStartup(brave_vpn::GetBraveVpnWireguardServiceName().c_str(),
                   service_cmd);
    }
    return success;
  }
  return false;
}

// Uninstalling and clearing Brave VPN service data.
bool UninstallBraveWireguardService() {
  brave_vpn::wireguard::RemoveExistingWireguardService();
  auto last_used_config = brave_vpn::wireguard::GetLastUsedConfigPath();
  if (last_used_config.has_value() &&
      !RemoveWireguardConfigDirectory(last_used_config.value())) {
    LOG(WARNING) << "Failed to delete config directory"
                 << last_used_config.value().DirName();
  }
  RemoveFromStartup(brave_vpn::GetBraveVpnWireguardServiceName().c_str());
  wireguard::RemoveStorageKey();

  if (!installer::InstallServiceWorkItem::DeleteService(
          brave_vpn::GetBraveVpnWireguardServiceName(),
          brave_vpn::wireguard::
              GetBraveVpnWireguardServiceRegistryStoragePath(),
          {}, {})) {
    LOG(WARNING) << "Failed to delete "
                 << brave_vpn::GetBraveVpnWireguardServiceName();
    return false;
  }
  return true;
}

}  // namespace brave_vpn
