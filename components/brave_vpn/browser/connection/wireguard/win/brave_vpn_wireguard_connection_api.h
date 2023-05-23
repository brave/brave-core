/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_CONNECTION_API_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_CONNECTION_API_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"

#include "brave/components/brave_vpn/browser/connection/wireguard/brave_vpn_wireguard_connection_api_base.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/common/brave_vpn_wireguard_profile_credentials.h"

class PrefService;

namespace brave_vpn {

class BraveVPNWireguardConnectionAPI
    : public BraveVPNWireguardConnectionAPIBase {
 public:
  BraveVPNWireguardConnectionAPI(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* local_prefs);

  BraveVPNWireguardConnectionAPI(const BraveVPNWireguardConnectionAPI&) =
      delete;
  BraveVPNWireguardConnectionAPI& operator=(
      const BraveVPNWireguardConnectionAPI&) = delete;
  ~BraveVPNWireguardConnectionAPI() override;

  // BraveVPNOSConnectionAPI
  void Disconnect() override;
  void CheckConnection() override;

 protected:
  // BraveVPNOSConnectionAPIBase
  void RequestNewProfileCredentials() override;
  void PlatformConnectImpl(
      const wireguard::WireguardProfileCredentials& credentials) override;

 private:
  base::WeakPtrFactory<BraveVPNWireguardConnectionAPI> weak_factory_{this};
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_CONNECTION_API_H_
