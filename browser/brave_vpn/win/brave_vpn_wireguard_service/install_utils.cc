/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/install_utils.h"

#include <windows.h>
#include <winerror.h>
#include <winnt.h>
#include <winsvc.h>
#include <winuser.h>

#include <ios>

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/win/registry.h"
#include "base/win/windows_types.h"
#include "brave/browser/brave_vpn/win/brave_vpn_helper/brave_vpn_helper_constants.h"
#include "brave/browser/brave_vpn/win/brave_vpn_helper/brave_vpn_helper_utils.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/service/wireguard_tunnel_service.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/brave_vpn_tray_command_ids.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/status_icon/constants.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/status_icon/icon_utils.h"
#include "brave/browser/brave_vpn/win/service_constants.h"
#include "brave/browser/brave_vpn/win/service_details.h"
#include "brave/browser/brave_vpn/win/storage_utils.h"
#include "brave/components/brave_vpn/common/win/scoped_sc_handle.h"
#include "brave/components/brave_vpn/common/win/utils.h"
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

// Microsoft-Windows-NetworkProfile
// fbcfac3f-8459-419f-8e48-1f0b49cdb85e
constexpr GUID kNetworkProfileGUID = {
    0xfbcfac3f,
    0x8459,
    0x419f,
    {0x8e, 0x48, 0x1f, 0x0b, 0x49, 0xcd, 0xb8, 0x5e}};

bool SetServiceTriggerForVPNConnection(SC_HANDLE hService,
                                       const std::wstring& brave_vpn_entry) {
  std::wstring brave_vpn_entry_with_null(brave_vpn_entry);
  brave_vpn_entry_with_null += L'\0';
  // Allocate and set the SERVICE_TRIGGER_SPECIFIC_DATA_ITEM structure
  SERVICE_TRIGGER_SPECIFIC_DATA_ITEM deviceData = {0};
  deviceData.dwDataType = SERVICE_TRIGGER_DATA_TYPE_STRING;
  // Exclude EOL
  deviceData.cbData = brave_vpn_entry_with_null.size() *
                      sizeof(brave_vpn_entry_with_null.front());
  deviceData.pData = (PBYTE)brave_vpn_entry_with_null.c_str();
  // Allocate and set the SERVICE_TRIGGER structure
  SERVICE_TRIGGER serviceTrigger = {0};
  serviceTrigger.dwTriggerType = SERVICE_TRIGGER_TYPE_CUSTOM;
  serviceTrigger.dwAction = SERVICE_TRIGGER_ACTION_SERVICE_START;
  serviceTrigger.pTriggerSubtype = const_cast<GUID*>(&kNetworkProfileGUID);
  serviceTrigger.cDataItems = 1;
  serviceTrigger.pDataItems = &deviceData;

  // Allocate and set the SERVICE_TRIGGER_INFO structure
  SERVICE_TRIGGER_INFO serviceTriggerInfo = {0};
  serviceTriggerInfo.cTriggers = 1;
  serviceTriggerInfo.pTriggers = &serviceTrigger;

  // Call ChangeServiceConfig2 with the SERVICE_CONFIG_TRIGGER_INFO level
  // and pass to it the address of the SERVICE_TRIGGER_INFO structure
  return ChangeServiceConfig2(hService, SERVICE_CONFIG_TRIGGER_INFO,
                              &serviceTriggerInfo);
}

bool ConfigureServiceAutoRestart(const std::wstring& service_name,
                                 const std::wstring& brave_vpn_entry) {
  ScopedScHandle scm(::OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT));
  if (!scm.IsValid()) {
    VLOG(1) << "::OpenSCManager failed. service_name: " << service_name
            << ", error: " << std::hex << HRESULTFromLastError();
    return false;
  }
  ScopedScHandle service(
      ::OpenService(scm.Get(), service_name.c_str(), SERVICE_ALL_ACCESS));
  if (!service.IsValid()) {
    VLOG(1) << "::OpenService failed. service_name: " << service_name
            << ", error: " << std::hex << HRESULTFromLastError();
    return false;
  }

  if (!brave_vpn::SetServiceFailureActions(service.Get())) {
    VLOG(1) << "SetServiceFailureActions failed:" << std::hex
            << HRESULTFromLastError();
    return false;
  }
  if (!SetServiceTriggerForVPNConnection(service.Get(), brave_vpn_entry)) {
    VLOG(1) << "SetServiceTriggerForVPNConnection failed:" << std::hex
            << HRESULTFromLastError();
    return false;
  }
  return true;
}

base::FilePath GetBraveVpnHelperServicePath(const base::FilePath& root_dir) {
  return root_dir.Append(brave_vpn::kBraveVPNHelperExecutable);
}

}  // namespace

bool ConfigureBraveWireguardService(const std::wstring& service_name) {
  ScopedScHandle scm(::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS));
  if (!scm.IsValid()) {
    VLOG(1) << "::OpenSCManager failed. service_name: " << service_name
            << ", error: " << std::hex << HRESULTFromLastError();
    return false;
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
bool InstallBraveWireguardService(const base::FilePath& root_dir) {
  base::CommandLine service_cmd(
      GetBraveVPNWireguardServiceExecutablePath(root_dir));
  installer::InstallServiceWorkItem install_service_work_item(
      brave_vpn::GetBraveVpnWireguardServiceName(),
      brave_vpn::GetBraveVpnWireguardServiceDisplayName(),
      brave_vpn::GetBraveVpnWireguardServiceDescription(), SERVICE_DEMAND_START,
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

bool UninstallStatusTrayIcon() {
  auto* hWnd = GetBraveVpnStatusTrayIconHWND();
  if (!hWnd) {
    return true;
  }

  return SendMessage(hWnd,
                     RegisterWindowMessage(kBraveVpnStatusTrayMessageName),
                     IDC_BRAVE_VPN_TRAY_EXIT, 0) == TRUE;
}

bool InstallBraveVPNHelperService(const base::FilePath& root_dir) {
  base::CommandLine service_cmd(GetBraveVpnHelperServicePath(root_dir));
  installer::InstallServiceWorkItem install_service_work_item(
      brave_vpn::GetBraveVpnHelperServiceName(),
      brave_vpn::GetBraveVpnHelperServiceDisplayName(),
      brave_vpn::GetBraveVpnHelperServiceDescription(), SERVICE_DEMAND_START,
      service_cmd, base::CommandLine(base::CommandLine::NO_PROGRAM),
      GetBraveVpnHelperRegistryStoragePath(), {}, {});
  install_service_work_item.set_best_effort(true);
  install_service_work_item.set_rollback_enabled(false);
  if (install_service_work_item.Do()) {
    auto success =
        ConfigureServiceAutoRestart(brave_vpn::GetBraveVpnHelperServiceName(),
                                    brave_vpn::GetBraveVPNConnectionName());
    return success;
  }
  return false;
}

}  // namespace brave_vpn
