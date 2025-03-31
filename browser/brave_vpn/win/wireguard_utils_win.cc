/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/wireguard_utils_win.h"

#include <objbase.h>

#include <stdint.h>
#include <wrl/client.h>

#include <optional>
#include <utility>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/single_thread_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/win/com_init_util.h"
#include "base/win/registry.h"
#include "base/win/scoped_bstr.h"
#include "brave/browser/brave_vpn/win/service_constants.h"
#include "brave/browser/brave_vpn/win/service_details.h"
#include "brave/components/brave_vpn/common/win/utils.h"
#include "brave/components/brave_vpn/common/wireguard/win/brave_wireguard_manager_idl.h"

namespace brave_vpn {

namespace {
std::optional<bool> g_wireguard_service_registered_for_testing;
std::optional<std::string> g_smart_proxy_url;
}  // namespace

namespace wireguard {

bool IsWireguardServiceInstalled() {
  if (g_wireguard_service_registered_for_testing.has_value()) {
    return g_wireguard_service_registered_for_testing.value();
  }
  return brave_vpn::GetWindowsServiceStatus(
             brave_vpn::GetBraveVpnWireguardServiceName())
      .has_value();
}

void SetWireguardServiceRegisteredForTesting(bool value) {
  g_wireguard_service_registered_for_testing = value;
}

bool IsBraveVPNWireguardTunnelServiceRunning() {
  auto status =
      GetWindowsServiceStatus(GetBraveVpnWireguardTunnelServiceName());
  if (!status.has_value()) {
    return false;
  }
  return status.value() == SERVICE_RUNNING ||
         status.value() == SERVICE_START_PENDING;
}

bool EnableBraveVpnWireguardServiceImpl(
    const std::string& server_public_key,
    const std::string& client_private_key,
    const std::string& mapped_ip4_address,
    const std::string& vpn_server_hostname) {
  base::win::AssertComInitialized();

  // TODO(bsclifton): clean this up
  if (g_smart_proxy_url.has_value()) {
    base::win::RegKey key;
    LONG rv = key.Create(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings",
        KEY_ALL_ACCESS);
    if (rv == ERROR_SUCCESS && key.Valid()) {
      std::wstring current_proxy_url;
      if (key.ReadValue(L"AutoConfigURL", &current_proxy_url) ==
          ERROR_SUCCESS) {
        LOG(ERROR) << L"User has `AutoConfigURL` value already set: \""
                   << current_proxy_url
                   << "\".\nBrave VPN smart proxy routing can't be used.";
      } else {
        // make the change
        current_proxy_url = base::UTF8ToWide(*g_smart_proxy_url);
        rv = key.WriteValue(L"AutoConfigURL", current_proxy_url.c_str());
        LOG(ERROR) << L"Set Brave VPN smart proxy routing to = \""
                   << current_proxy_url << "\"";
      }
      // TODO(bsclifton): if value passed in is empty, delete key
    }
  }
  // ------------------------------

  Microsoft::WRL::ComPtr<IBraveVpnWireguardManager> service;
  if (FAILED(CoCreateInstance(brave_vpn::GetBraveVpnWireguardServiceClsid(),
                              nullptr, CLSCTX_LOCAL_SERVER,
                              brave_vpn::GetBraveVpnWireguardServiceIid(),
                              IID_PPV_ARGS_Helper(&service)))) {
    VLOG(1) << "Unable to create IBraveVpnWireguardManager instance";
    return false;
  }

  if (FAILED(CoSetProxyBlanket(
          service.Get(), RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT,
          COLE_DEFAULT_PRINCIPAL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
          RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_DYNAMIC_CLOAKING))) {
    VLOG(1) << "Unable to call EnableVpn interface";
    return false;
  }

  base::win::ScopedBstr server_public_key_data(
      base::UTF8ToWide(server_public_key));
  base::win::ScopedBstr client_private_key_data(
      base::UTF8ToWide(client_private_key));
  base::win::ScopedBstr mapped_ip4_address_data(
      base::UTF8ToWide(mapped_ip4_address));
  base::win::ScopedBstr vpn_server_hostname_data(
      base::UTF8ToWide(vpn_server_hostname));

  DWORD last_error = ERROR_SUCCESS;
  HRESULT res = service->EnableVpn(server_public_key_data.Get(),
                                   client_private_key_data.Get(),
                                   mapped_ip4_address_data.Get(),
                                   vpn_server_hostname_data.Get(), &last_error);

  if (!SUCCEEDED(res)) {
    VLOG(1) << "Failure calling EnableVpn. Result: "
            << logging::SystemErrorCodeToString(res)
            << " GetLastError: " << last_error;
    return false;
  }

  return true;
}

void EnableBraveVpnWireguardService(const std::string& server_public_key,
                                    const std::string& client_private_key,
                                    const std::string& mapped_ip4_address,
                                    const std::string& vpn_server_hostname,
                                    std::optional<std::string> smart_proxy_url,
                                    wireguard::BooleanCallback callback) {
  g_smart_proxy_url.reset();
  if (smart_proxy_url.has_value()) {
    g_smart_proxy_url = smart_proxy_url;
  }

  base::ThreadPool::CreateCOMSTATaskRunner(
      {base::MayBlock(), base::WithBaseSyncPrimitives(),
       base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::SingleThreadTaskRunnerThreadMode::DEDICATED)
      ->PostTaskAndReplyWithResult(
          FROM_HERE,
          base::BindOnce(&EnableBraveVpnWireguardServiceImpl, server_public_key,
                         client_private_key, mapped_ip4_address,
                         vpn_server_hostname),
          std::move(callback));
}

bool DisableBraveVpnWireguardServiceImpl() {
  base::win::AssertComInitialized();

  // TODO(bsclifton): clean this up
  base::win::RegKey key;
  LONG rv = key.Create(
      HKEY_CURRENT_USER,
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings",
      KEY_ALL_ACCESS);
  if (rv == ERROR_SUCCESS && key.Valid()) {
    // TODO(bsclifton): only delete if enabled via above
    key.DeleteValue(L"AutoConfigURL");
    LOG(ERROR) << "BSC]] delete";
  }
  // ------------------------------

  Microsoft::WRL::ComPtr<IBraveVpnWireguardManager> service;
  if (FAILED(CoCreateInstance(brave_vpn::GetBraveVpnWireguardServiceClsid(),
                              nullptr, CLSCTX_LOCAL_SERVER,
                              brave_vpn::GetBraveVpnWireguardServiceIid(),
                              IID_PPV_ARGS_Helper(&service)))) {
    VLOG(1) << "Unable to create IBraveVpnService instance";
    return false;
  }

  if (FAILED(CoSetProxyBlanket(
          service.Get(), RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT,
          COLE_DEFAULT_PRINCIPAL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
          RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_DYNAMIC_CLOAKING))) {
    VLOG(1) << "Unable to call EnableVpn interface";
    return false;
  }

  DWORD last_error = ERROR_SUCCESS;
  HRESULT res = service->DisableVpn(&last_error);
  if (!SUCCEEDED(res)) {
    VLOG(1) << "Failure calling DisableVpn. Result: "
            << logging::SystemErrorCodeToString(res)
            << " GetLastError: " << last_error;
    return false;
  }

  return true;
}

void DisableBraveVpnWireguardService(wireguard::BooleanCallback callback) {
  base::ThreadPool::CreateCOMSTATaskRunner(
      {base::MayBlock(), base::WithBaseSyncPrimitives(),
       base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::SingleThreadTaskRunnerThreadMode::DEDICATED)
      ->PostTaskAndReplyWithResult(
          FROM_HERE, base::BindOnce(&DisableBraveVpnWireguardServiceImpl),
          std::move(callback));
}

wireguard::WireguardKeyPair WireguardGenerateKeypairImpl() {
  base::win::AssertComInitialized();
  Microsoft::WRL::ComPtr<IBraveVpnWireguardManager> service;
  if (FAILED(CoCreateInstance(brave_vpn::GetBraveVpnWireguardServiceClsid(),
                              nullptr, CLSCTX_LOCAL_SERVER,
                              brave_vpn::GetBraveVpnWireguardServiceIid(),
                              IID_PPV_ARGS_Helper(&service)))) {
    VLOG(1) << "Unable to create IBraveVpnWireguardManager instance";
    return std::nullopt;
  }

  if (FAILED(CoSetProxyBlanket(
          service.Get(), RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT,
          COLE_DEFAULT_PRINCIPAL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
          RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_DYNAMIC_CLOAKING))) {
    VLOG(1) << "Unable to call CoSetProxyBlanket";
    return std::nullopt;
  }

  DWORD last_error = ERROR_SUCCESS;
  HRESULT res;
  base::win::ScopedBstr public_key_data;
  base::win::ScopedBstr private_key_data;
  res = service->GenerateKeypair(public_key_data.Receive(),
                                 private_key_data.Receive(), &last_error);

  if (!SUCCEEDED(res)) {
    VLOG(1) << "Failure calling GenerateKeyPair. Result: "
            << logging::SystemErrorCodeToString(res)
            << " GetLastError: " << last_error;
    return std::nullopt;
  }

  std::string public_key = reinterpret_cast<std::string::value_type*>(
      public_key_data.Get(), public_key_data.ByteLength());
  std::string private_key = reinterpret_cast<std::string::value_type*>(
      private_key_data.Get(), private_key_data.ByteLength());
  return std::make_tuple(public_key, private_key);
}

void WireguardGenerateKeypair(
    wireguard::WireguardGenerateKeypairCallback callback) {
  base::ThreadPool::CreateCOMSTATaskRunner(
      {base::MayBlock(), base::WithBaseSyncPrimitives(),
       base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::SingleThreadTaskRunnerThreadMode::DEDICATED)
      ->PostTaskAndReplyWithResult(
          FROM_HERE, base::BindOnce(&WireguardGenerateKeypairImpl),
          std::move(callback));
}

void ShowBraveVpnStatusTrayIcon() {
  auto executable_path = brave_vpn::GetBraveVPNWireguardServiceExecutablePath(
      base::PathService::CheckedGet(base::DIR_ASSETS));
  base::CommandLine interactive_cmd(executable_path);
  interactive_cmd.AppendSwitch(
      brave_vpn::kBraveVpnWireguardServiceInteractiveSwitchName);
  if (!base::LaunchProcess(interactive_cmd, base::LaunchOptions()).IsValid()) {
    VLOG(1) << "Interactive process launch failed";
  }
}

}  // namespace wireguard

}  // namespace brave_vpn
