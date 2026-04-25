/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_MAC_WIREGUARD_CONNECTION_API_IMPL_MAC_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_MAC_WIREGUARD_CONNECTION_API_IMPL_MAC_H_

#include "brave/components/brave_vpn/browser/connection/wireguard/wireguard_connection_api_impl_base.h"

namespace brave_vpn {

class WireguardConnectionAPIImplMac : public WireguardConnectionAPIImplBase {
 public:
  using WireguardConnectionAPIImplBase::WireguardConnectionAPIImplBase;
  ~WireguardConnectionAPIImplMac() override;

  // WireguardConnectionAPIImplBase
  void Disconnect() override;
  void CheckConnection() override;

  // BraveVPNWireguardConnectionAPIBase overrides:
  void PlatformConnectImpl(
      const wireguard::WireguardProfileCredentials& credentials) override;
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_MAC_WIREGUARD_CONNECTION_API_IMPL_MAC_H_
