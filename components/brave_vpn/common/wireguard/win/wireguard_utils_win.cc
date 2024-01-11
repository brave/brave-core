/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/common/wireguard/win/wireguard_utils_win.h"

#include <objbase.h>
#include <stdint.h>
#include <wrl/client.h>

#include <optional>
#include <utility>

#include "base/base64.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/process/launch.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/single_thread_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/win/com_init_util.h"
#include "base/win/scoped_bstr.h"
#include "brave/components/brave_vpn/common/win/utils.h"
#include "brave/components/brave_vpn/common/wireguard/win/brave_wireguard_manager_idl.h"
#include "brave/components/brave_vpn/common/wireguard/win/service_constants.h"
#include "brave/components/brave_vpn/common/wireguard/win/service_details.h"

namespace brave_vpn {

namespace {
std::optional<bool> g_wireguard_service_registered_for_testing;
}  // namespace

namespace wireguard {

bool IsWireguardServiceInstalled(version_info::Channel channel) {
  if (g_wireguard_service_registered_for_testing.has_value()) {
    return g_wireguard_service_registered_for_testing.value();
  }
  return brave_vpn::GetWindowsServiceStatus(
             brave_vpn::GetBraveVpnWireguardServiceName(channel))
      .has_value();
}

void SetWireguardServiceRegisteredForTesting(bool value) {
  g_wireguard_service_registered_for_testing = value;
}

bool IsBraveVPNWireguardTunnelServiceRunning(version_info::Channel channel) {
  auto status =
      GetWindowsServiceStatus(GetBraveVpnWireguardTunnelServiceName(channel));
  if (!status.has_value()) {
    return false;
  }
  return status.value() == SERVICE_RUNNING ||
         status.value() == SERVICE_START_PENDING;
}

bool EnableBraveVpnWireguardServiceImpl(const std::string& config,
                                        version_info::Channel channel) {
  base::win::AssertComInitialized();
  Microsoft::WRL::ComPtr<IBraveVpnWireguardManager> service;
  if (FAILED(CoCreateInstance(
          brave_vpn::GetBraveVpnWireguardServiceClsid(channel), nullptr,
          CLSCTX_LOCAL_SERVER, brave_vpn::GetBraveVpnWireguardServiceIid(),
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
  std::string encoded_config;
  base::Base64Encode(config, &encoded_config);
  DWORD error_code = 0;
  if (FAILED(service->EnableVpn(base::UTF8ToWide(encoded_config).c_str(),
                                &error_code))) {
    VLOG(1) << "Unable to call EnableVpn interface";
    return false;
  }
  return error_code == 0;
}

void EnableBraveVpnWireguardService(const std::string& config,
                                    version_info::Channel channel,
                                    wireguard::BooleanCallback callback) {
  base::ThreadPool::CreateCOMSTATaskRunner(
      {base::MayBlock(), base::WithBaseSyncPrimitives(),
       base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::SingleThreadTaskRunnerThreadMode::DEDICATED)
      ->PostTaskAndReplyWithResult(
          FROM_HERE,
          base::BindOnce(&EnableBraveVpnWireguardServiceImpl, config, channel),
          std::move(callback));
}

bool DisableBraveVpnWireguardServiceImpl(version_info::Channel channel) {
  base::win::AssertComInitialized();

  Microsoft::WRL::ComPtr<IBraveVpnWireguardManager> service;
  if (FAILED(CoCreateInstance(
          brave_vpn::GetBraveVpnWireguardServiceClsid(channel), nullptr,
          CLSCTX_LOCAL_SERVER, brave_vpn::GetBraveVpnWireguardServiceIid(),
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
  DWORD error_code = 0;
  if (FAILED(service->DisableVpn(&error_code))) {
    VLOG(1) << "Unable to call EnableVpn interface";
    return false;
  }
  return error_code == 0;
}

void DisableBraveVpnWireguardService(version_info::Channel channel,
                                     wireguard::BooleanCallback callback) {
  base::ThreadPool::CreateCOMSTATaskRunner(
      {base::MayBlock(), base::WithBaseSyncPrimitives(),
       base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::SingleThreadTaskRunnerThreadMode::DEDICATED)
      ->PostTaskAndReplyWithResult(
          FROM_HERE,
          base::BindOnce(&DisableBraveVpnWireguardServiceImpl, channel),
          std::move(callback));
}

wireguard::WireguardKeyPair WireguardGenerateKeypairImpl(
    version_info::Channel channel) {
  base::win::AssertComInitialized();
  Microsoft::WRL::ComPtr<IBraveVpnWireguardManager> service;
  if (FAILED(CoCreateInstance(
          brave_vpn::GetBraveVpnWireguardServiceClsid(channel), nullptr,
          CLSCTX_LOCAL_SERVER, brave_vpn::GetBraveVpnWireguardServiceIid(),
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

  DWORD error_code = 0;
  base::win::ScopedBstr public_key_raw;
  base::win::ScopedBstr private_key_raw;
  if (FAILED(service->GenerateKeypair(
          public_key_raw.Receive(), private_key_raw.Receive(), &error_code)) ||
      error_code) {
    VLOG(1) << "Unable to generate keypair";
    return std::nullopt;
  }

  std::wstring public_key_wide;
  public_key_wide.assign(
      reinterpret_cast<std::wstring::value_type*>(public_key_raw.Get()),
      public_key_raw.Length());
  std::wstring private_key_wide;
  private_key_wide.assign(
      reinterpret_cast<std::wstring::value_type*>(private_key_raw.Get()),
      private_key_raw.Length());
  std::string public_key = base::WideToUTF8(public_key_wide);
  std::string private_key = base::WideToUTF8(private_key_wide);
  return std::make_tuple(public_key, private_key);
}

void WireguardGenerateKeypair(
    version_info::Channel channel,
    wireguard::WireguardGenerateKeypairCallback callback) {
  base::ThreadPool::CreateCOMSTATaskRunner(
      {base::MayBlock(), base::WithBaseSyncPrimitives(),
       base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::SingleThreadTaskRunnerThreadMode::DEDICATED)
      ->PostTaskAndReplyWithResult(
          FROM_HERE, base::BindOnce(&WireguardGenerateKeypairImpl, channel),
          std::move(callback));
}

void ShowBraveVpnStatusTrayIcon() {
  auto executable_path = brave_vpn::GetBraveVPNWireguardServiceExecutablePath();
  base::CommandLine interactive_cmd(executable_path);
  interactive_cmd.AppendSwitch(
      brave_vpn::kBraveVpnWireguardServiceInteractiveSwitchName);
  if (!base::LaunchProcess(interactive_cmd, base::LaunchOptions()).IsValid()) {
    VLOG(1) << "Interactive process launch failed";
  }
}

}  // namespace wireguard

}  // namespace brave_vpn
