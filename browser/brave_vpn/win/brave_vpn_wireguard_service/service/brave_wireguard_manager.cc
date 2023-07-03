/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/service/brave_wireguard_manager.h"

#include <string>
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/service/wireguard_tunnel_service.h"

namespace brave_vpn {

HRESULT BraveWireguardManager::EnableVpn(const wchar_t* config,
                                         DWORD* last_error) {
  if (!config || !last_error) {
    VLOG(1) << "Invalid parameters";
    return E_FAIL;
  }
  *last_error = !brave_vpn::wireguard::LaunchWireguardService(config);
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
