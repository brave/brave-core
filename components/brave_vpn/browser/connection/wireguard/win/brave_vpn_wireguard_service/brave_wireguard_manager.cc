/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/brave_wireguard_manager.h"

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/service_main.h"

namespace brave_vpn {

HRESULT BraveWireguardManager::EnableVpn(const wchar_t* config,
                                         DWORD* last_error) {
  if (!config || !last_error) {
    VLOG(1) << "Invalid parameters";
    return E_FAIL;
  }
  // TODO(spylogsster): Implement wireguard service call.
  return S_OK;
}

HRESULT BraveWireguardManager::DisableVpn(DWORD* last_error) {
  // TODO(spylogsster): Implement wireguard service call.
  return S_OK;
}

HRESULT BraveWireguardManager::GenerateKeypair(BSTR* public_key,
                                               BSTR* private_key,
                                               DWORD* last_error) {
  // TODO(spylogsster): Implement wireguard service call.
  return S_OK;
}

}  // namespace brave_vpn
