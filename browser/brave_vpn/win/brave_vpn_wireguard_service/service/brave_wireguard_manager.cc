/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/service/brave_wireguard_manager.h"

#include <string>

#include "base/base64.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/windows_types.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/service/wireguard_tunnel_service.h"
#include "brave/components/brave_vpn/common/wireguard/wireguard_utils.h"

namespace brave_vpn {

HRESULT BraveWireguardManager::EnableVpn(const BSTR public_key,
                                         const BSTR private_key,
                                         const BSTR address,
                                         const BSTR endpoint,
                                         DWORD* last_error) {
  // if all params are empty, reconnect using last known good config.
  // browser/brave_vpn/win/brave_vpn_wireguard_service/service/wireguard_tunnel_service.cc
  bool reconnect_using_last_config = public_key && wcslen(public_key) == 0 &&
                                     private_key && wcslen(private_key) == 0 &&
                                     address && wcslen(address) == 0 &&
                                     endpoint && wcslen(endpoint) == 0;
  if (reconnect_using_last_config) {
    if (!brave_vpn::wireguard::LaunchWireguardService(L"")) {
      *last_error = ::GetLastError();
      return E_FAIL;
    } else {
      return S_OK;
    }
  }

  std::string public_key_str;
  if (!base::WideToUTF8(public_key, wcslen(public_key), &public_key_str)) {
    VLOG(1) << "failed WideToUTF8 for public_key";
    return E_INVALIDARG;
  }
  auto validated_public_key =
      brave_vpn::wireguard::ValidateKey(public_key_str, "public_key");
  if (!validated_public_key.has_value()) {
    return E_INVALIDARG;
  }

  std::string private_key_str;
  if (!base::WideToUTF8(private_key, wcslen(private_key), &private_key_str)) {
    VLOG(1) << "failed WideToUTF8 for private_key";
    return E_INVALIDARG;
  }
  auto validated_private_key =
      brave_vpn::wireguard::ValidateKey(private_key_str, "private_key");
  if (!validated_private_key.has_value()) {
    return E_INVALIDARG;
  }

  std::string address_str;
  if (!base::WideToUTF8(address, wcslen(address), &address_str)) {
    VLOG(1) << "failed WideToUTF8 for address";
    return E_INVALIDARG;
  }
  auto validated_address = brave_vpn::wireguard::ValidateAddress(address_str);
  if (!validated_address.has_value()) {
    return E_INVALIDARG;
  }

  std::string endpoint_str;
  if (!base::WideToUTF8(endpoint, wcslen(endpoint), &endpoint_str)) {
    VLOG(1) << "failed WideToUTF8 for endpoint";
    return E_INVALIDARG;
  }
  auto validated_endpoint =
      brave_vpn::wireguard::ValidateEndpoint(endpoint_str);
  if (!validated_endpoint.has_value()) {
    return E_INVALIDARG;
  }

  auto config = brave_vpn::wireguard::CreateWireguardConfig(
      validated_private_key.value(), validated_public_key.value(),
      validated_endpoint.value(), validated_address.value());
  if (!config.has_value()) {
    VLOG(1) << __func__ << " : failed to get correct credentials";
    return E_INVALIDARG;
  }

  std::wstring config_str =
      base::UTF8ToWide(base::Base64Encode(config.value()));

  if (!brave_vpn::wireguard::LaunchWireguardService(config_str.c_str())) {
    *last_error = ::GetLastError();
    return E_FAIL;
  }

  return S_OK;
}

HRESULT BraveWireguardManager::DisableVpn(DWORD* last_error) {
  if (!brave_vpn::wireguard::RemoveExistingWireguardService()) {
    *last_error = ::GetLastError();
    return E_FAIL;
  }
  return S_OK;
}

HRESULT BraveWireguardManager::GenerateKeypair(BSTR* public_key,
                                               BSTR* private_key,
                                               DWORD* last_error) {
  if (!public_key || !private_key) {
    VLOG(1) << __func__ << ": unable to generate keys";
    return E_INVALIDARG;
  }
  std::string public_key_raw;
  std::string private_key_raw;
  if (!brave_vpn::wireguard::WireguardGenerateKeypair(&public_key_raw,
                                                      &private_key_raw)) {
    VLOG(1) << __func__ << ": unable to generate keys";
    *last_error = ::GetLastError();
    return E_INVALIDARG;
  }

  *public_key = SysAllocString(base::UTF8ToWide(public_key_raw).c_str());
  *private_key = SysAllocString(base::UTF8ToWide(private_key_raw).c_str());
  if (!*public_key || !*private_key) {
    return E_OUTOFMEMORY;
  }
  return S_OK;
}

}  // namespace brave_vpn
