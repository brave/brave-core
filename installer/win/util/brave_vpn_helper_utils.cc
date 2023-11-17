// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/installer/win/util/brave_vpn_helper_utils.h"

#include "base/files/file_util.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/com_init_util.h"
#include "base/win/registry.h"
#include "base/win/scoped_handle.h"
#include "base/win/windows_types.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/win/scoped_sc_handle.h"
#include "brave/components/brave_vpn/common/win/utils.h"
#include "brave/components/brave_vpn/common/wireguard/win/service_constants.h"
#include "brave/components/brave_vpn/common/wireguard/win/service_details.h"
#include "brave/components/brave_vpn/common/wireguard/win/wireguard_utils_win.h"
#include "brave/installer/win/util/brave_vpn_helper_constants.h"
#include "chrome/elevation_service/elevation_service_idl.h"
#include "chrome/install_static/install_modes.h"
#include "chrome/install_static/install_util.h"
#include "chrome/installer/util/install_service_work_item.h"
#include "components/version_info/version_info.h"
#include "third_party/abseil-cpp/absl/cleanup/cleanup.h"

namespace brave_vpn {

namespace {

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

}  // namespace

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

base::FilePath GetBraveVpnHelperServicePath() {
  base::FilePath asset_dir = base::PathService::CheckedGet(base::DIR_ASSETS);
  return asset_dir.Append(brave_vpn::kBraveVPNHelperExecutable);
}

// The service starts under system user so we save crashes to
// %PROGRAMDATA%\BraveSoftware\{service name}\Crashpad
base::FilePath GetVpnHelperServiceProfileDir() {
  std::wstring program_data =
      install_static::GetEnvironmentString(L"PROGRAMDATA");
  if (program_data.empty()) {
    return base::FilePath();
  }
  return base::FilePath(program_data)
      .Append(install_static::kCompanyPathName)
      .Append(brave_vpn::GetBraveVpnHelperServiceName());
}

bool InstallBraveVPNHelperService() {
  base::CommandLine service_cmd(GetBraveVpnHelperServicePath());
  installer::InstallServiceWorkItem install_service_work_item(
      brave_vpn::GetBraveVpnHelperServiceName(),
      brave_vpn::GetBraveVpnHelperServiceDisplayName(), SERVICE_DEMAND_START,
      service_cmd, base::CommandLine(base::CommandLine::NO_PROGRAM),
      brave_vpn::kBraveVpnHelperRegistryStoragePath, {}, {});
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

bool IsBraveVPNHelperServiceInstalled() {
  ScopedScHandle scm(::OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT));
  if (!scm.IsValid()) {
    VLOG(1) << "::OpenSCManager failed. service_name: "
            << brave_vpn::GetBraveVpnHelperServiceName()
            << ", error: " << std::hex << HRESULTFromLastError();
    return false;
  }
  ScopedScHandle service(::OpenService(
      scm.Get(), brave_vpn::GetBraveVpnHelperServiceName().c_str(),
      SERVICE_QUERY_STATUS));

  // Service registered and has not exceeded the number of auto-configured
  // restarts.
  return service.IsValid();
}

bool IsNetworkFiltersInstalled() {
  DCHECK(IsBraveVPNHelperServiceInstalled());
  base::win::RegKey service_storage_key(
      HKEY_LOCAL_MACHINE, brave_vpn::kBraveVpnHelperRegistryStoragePath,
      KEY_READ);
  if (!service_storage_key.Valid()) {
    return false;
  }
  DWORD current = -1;
  if (service_storage_key.ReadValueDW(
          brave_vpn::kBraveVpnHelperFiltersInstalledValue, &current) !=
      ERROR_SUCCESS) {
    return false;
  }
  return current > 0;
}

HRESULT InstallBraveVPNHelperServiceImpersonated() {
  HRESULT hr = ::CoImpersonateClient();
  if (FAILED(hr)) {
    return hr;
  }

  absl::Cleanup cleanup([]() { ::CoRevertToSelf(); });

  auto executable_path = brave_vpn::GetBraveVpnHelperServicePath();
  base::CommandLine command_line(executable_path);
  command_line.AppendSwitch(brave_vpn::kBraveVpnHelperInstall);

  base::LaunchOptions options = base::LaunchOptions();
  options.feedback_cursor_off = true;
  options.wait = true;
  base::Process proc = base::LaunchProcess(command_line, options);
  if (!proc.IsValid()) {
    return brave_vpn::HRESULTFromLastError();
  }

  return S_OK;
}

HRESULT InstallBraveWireGuardServiceImpersonated() {
  HRESULT hr = ::CoImpersonateClient();
  if (FAILED(hr)) {
    return hr;
  }

  absl::Cleanup cleanup([]() { ::CoRevertToSelf(); });

  auto executable_path = brave_vpn::GetBraveVPNWireguardServiceExecutablePath();
  base::CommandLine command_line(executable_path);
  command_line.AppendSwitch(
      brave_vpn::kBraveVpnWireguardServiceInstallSwitchName);

  base::LaunchOptions options = base::LaunchOptions();
  options.feedback_cursor_off = true;
  options.wait = true;
  base::Process proc = base::LaunchProcess(command_line, options);
  if (!proc.IsValid()) {
    return brave_vpn::HRESULTFromLastError();
  }

  return S_OK;
}

std::wstring GetBraveVPNConnectionName() {
  return base::UTF8ToWide(
      brave_vpn::GetBraveVPNEntryName(install_static::GetChromeChannel()));
}

std::wstring GetBraveVpnHelperServiceDisplayName() {
  static constexpr wchar_t kBraveVpnServiceDisplayName[] = L" Vpn Service";
  return install_static::GetBaseAppName() + kBraveVpnServiceDisplayName;
}

std::wstring GetBraveVpnHelperServiceName() {
  std::wstring name = GetBraveVpnHelperServiceDisplayName();
  name.erase(std::remove_if(name.begin(), name.end(), isspace), name.end());
  return name;
}

bool InstallVPNSystemServices() {
  base::win::AssertComInitialized();

  Microsoft::WRL::ComPtr<IElevator> elevator;
  HRESULT hr = CoCreateInstance(
      install_static::GetElevatorClsid(), nullptr, CLSCTX_LOCAL_SERVER,
      install_static::GetElevatorIid(), IID_PPV_ARGS_Helper(&elevator));
  if (FAILED(hr)) {
    LOG(ERROR) << "CoCreateInstance returned: 0x" << std::hex << hr;
    return false;
  }

  hr = CoSetProxyBlanket(
      elevator.Get(), RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT,
      COLE_DEFAULT_PRINCIPAL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
      RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_DYNAMIC_CLOAKING);
  if (FAILED(hr)) {
    LOG(ERROR) << "CoSetProxyBlanket returned: 0x" << std::hex << hr;
    return false;
  }

  hr = elevator->InstallVPNServices();
  if (FAILED(hr)) {
    LOG(ERROR) << "InstallVPNServices returned: 0x" << std::hex << hr;
    return false;
  }

  VLOG(1) << "InstallVPNServices: SUCCESS";
  return true;
}

}  // namespace brave_vpn
