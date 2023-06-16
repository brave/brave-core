/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/install_utils.h"

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/win/windows_types.h"
#include "brave/components/brave_vpn/browser/connection/common/win/scoped_sc_handle.h"
#include "brave/components/brave_vpn/browser/connection/common/win/utils.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/common/service_constants.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/common/wireguard_utils.h"
#include "chrome/installer/util/install_service_work_item.h"

namespace brave_vpn {

namespace {
bool RemoveWireguardConfigDirectory(const base::FilePath& last_used_config) {
  auto wireguard_config_folder = last_used_config.DirName();
  if (wireguard_config_folder.empty() ||
      !base::PathExists(wireguard_config_folder)) {
    return true;
  }

  return base::DeletePathRecursively(wireguard_config_folder);
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
      brave_vpn::GetBraveVpnWireguardServiceRegistryStoragePath(),
      {brave_vpn::GetBraveVpnWireguardServiceClsid()},
      {brave_vpn::GetBraveVpnWireguardServiceIid()});
  install_service_work_item.set_best_effort(true);
  install_service_work_item.set_rollback_enabled(false);
  if (install_service_work_item.Do()) {
    return brave_vpn::ConfigureBraveWireguardService(
        brave_vpn::GetBraveVpnWireguardServiceName());
  }
  return false;
}

// Uninstalling and clearing Brave VPN service data.
bool UninstallBraveWireguardService() {
  auto last_used_config = brave_vpn::wireguard::GetLastUsedConfigPath();
  if (last_used_config.has_value() &&
      !RemoveWireguardConfigDirectory(last_used_config.value())) {
    LOG(WARNING) << "Failed to delete config directory"
                 << last_used_config.value().DirName();
  }

  if (!installer::InstallServiceWorkItem::DeleteService(
          brave_vpn::GetBraveVpnWireguardServiceName(),
          brave_vpn::GetBraveVpnWireguardServiceRegistryStoragePath(), {},
          {})) {
    LOG(WARNING) << "Failed to delete "
                 << brave_vpn::GetBraveVpnWireguardServiceName();
    return false;
  }
  return true;
}

}  // namespace brave_vpn
