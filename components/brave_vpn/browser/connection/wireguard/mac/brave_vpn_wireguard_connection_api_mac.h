/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_MAC_BRAVE_VPN_WIREGUARD_CONNECTION_API_MAC_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_MAC_BRAVE_VPN_WIREGUARD_CONNECTION_API_MAC_H_

#include "base/memory/scoped_refptr.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/brave_vpn_wireguard_connection_api_base.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/credentials/brave_vpn_wireguard_profile_credentials.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_vpn {

class WireguardOSConnectionAPIMac : public BraveVPNWireguardConnectionAPIBase {
 public:
  WireguardOSConnectionAPIMac(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* local_prefs);

  WireguardOSConnectionAPIMac(const WireguardOSConnectionAPIMac&) = delete;
  WireguardOSConnectionAPIMac& operator=(const WireguardOSConnectionAPIMac&) =
      delete;
  ~WireguardOSConnectionAPIMac() override;

 private:
  // BraveVPNOSConnectionAPI
  void Disconnect() override;
  void CheckConnection() override;

  // BraveVPNWireguardConnectionAPIBase overrides:
  void PlatformConnectImpl(
      const wireguard::WireguardProfileCredentials& credentials) override;
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_MAC_BRAVE_VPN_WIREGUARD_CONNECTION_API_MAC_H_
