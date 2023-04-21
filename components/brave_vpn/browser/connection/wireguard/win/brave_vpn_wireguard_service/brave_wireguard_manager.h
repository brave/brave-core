/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_SERVICE_BRAVE_WIREGUARD_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_SERVICE_BRAVE_WIREGUARD_MANAGER_H_

#include <wrl/implements.h>
#include <wrl/module.h>

#include <windows.h>

#include "base/win/windows_types.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/brave_wireguard_manager_idl.h"

namespace brave_vpn {

class BraveWireguardManager
    : public Microsoft::WRL::RuntimeClass<
          Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
          IBraveWireguardManager> {
 public:
  BraveWireguardManager() = default;

  BraveWireguardManager(const BraveWireguardManager&) = delete;
  BraveWireguardManager& operator=(const BraveWireguardManager&) = delete;

  IFACEMETHODIMP EnableVpn(const wchar_t* config, DWORD* last_error) override;
  IFACEMETHODIMP DisableVpn(DWORD* last_error) override;
  IFACEMETHODIMP GenerateKeypair(BSTR* public_key,
                                 BSTR* private_key,
                                 DWORD* last_error) override;

 private:
  ~BraveWireguardManager() override = default;
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_SERVICE_BRAVE_WIREGUARD_MANAGER_H_
