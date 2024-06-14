/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/service/brave_wireguard_manager.h"

#include <string>

#include "base/base64.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/service/wireguard_tunnel_service.h"
#include "brave/components/brave_vpn/common/wireguard/wireguard_utils.h"

namespace brave_vpn {

HRESULT BraveWireguardManager::EnableVpn(BSTR public_key,
                                         BSTR private_key,
                                         BSTR address,
                                         BSTR endpoint,
                                         DWORD* last_error) {
  if (!last_error) {
    VLOG(1) << "last_error must be set";
    return E_FAIL;
  }

  // if all params are empty, reconnect using last known good config.
  // browser/brave_vpn/win/brave_vpn_wireguard_service/service/wireguard_tunnel_service.cc
  bool reconnect_using_last_config = public_key && wcslen(public_key) == 0 &&
                                     private_key && wcslen(private_key) == 0 &&
                                     address && wcslen(address) == 0 &&
                                     endpoint && wcslen(endpoint) == 0;
  if (reconnect_using_last_config) {
    *last_error = !brave_vpn::wireguard::LaunchWireguardService(L"");
    return S_OK;
  }

  std::string public_key_str;
  if (!brave_vpn::wireguard::ValidateKey(public_key, &public_key_str,
                                         "public_key")) {
    return E_FAIL;
  }

  std::string private_key_str;
  if (!brave_vpn::wireguard::ValidateKey(private_key, &private_key_str,
                                         "private_key")) {
    return E_FAIL;
  }

  std::string address_str;
  if (!brave_vpn::wireguard::ValidateAddress(address, &address_str)) {
    return E_FAIL;
  }

  std::string endpoint_str;
  if (!brave_vpn::wireguard::ValidateEndpoint(endpoint, &endpoint_str)) {
    return E_FAIL;
  }

  auto config = brave_vpn::wireguard::CreateWireguardConfig(
      private_key_str, public_key_str, endpoint_str, address_str);
  if (!config.has_value()) {
    VLOG(1) << __func__ << " : failed to get correct credentials";
    return E_FAIL;
  }

  std::wstring config_str =
      base::UTF8ToWide(base::Base64Encode(config.value()));
  *last_error =
      !brave_vpn::wireguard::LaunchWireguardService(config_str.c_str());
  return S_OK;
}

HRESULT BraveWireguardManager::DisableVpn(DWORD* last_error) {
  *last_error = !brave_vpn::wireguard::RemoveExistingWireguardService();
  return S_OK;
}

HRESULT BraveWireguardManager::GenerateKeypair(BSTR* public_key,
                                               BSTR* private_key,
                                               DWORD* last_error) {
  if (!public_key || !private_key || !last_error) {
    VLOG(1) << __func__ << ": unable to generate keys";
    return E_FAIL;
  }
  std::string public_key_raw;
  std::string private_key_raw;
  *last_error = !brave_vpn::wireguard::WireguardGenerateKeypair(
      &public_key_raw, &private_key_raw);
  if (*last_error) {
    VLOG(1) << __func__ << ": unable to generate keys";
    return S_OK;
  }

  *public_key = ::SysAllocString(base::UTF8ToWide(public_key_raw).c_str());
  *private_key = ::SysAllocString(base::UTF8ToWide(private_key_raw).c_str());
  return S_OK;
}

}  // namespace brave_vpn
