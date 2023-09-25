/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_BRAVE_VPN_WIREGUARD_CONNECTION_API_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_BRAVE_VPN_WIREGUARD_CONNECTION_API_BASE_H_

#include <string>

#include "brave/components/brave_vpn/browser/connection/brave_vpn_os_connection_api.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/credentials/brave_vpn_wireguard_profile_credentials.h"
#include "brave/components/brave_vpn/common/wireguard/wireguard_utils.h"

class PrefService;

namespace brave_vpn {

class BraveVPNWireguardConnectionAPIBase
    : public BraveVPNOSConnectionAPI,
      public BraveVPNOSConnectionAPI::Observer {
 public:
  BraveVPNWireguardConnectionAPIBase(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* local_prefs);

  BraveVPNWireguardConnectionAPIBase(
      const BraveVPNWireguardConnectionAPIBase&) = delete;
  BraveVPNWireguardConnectionAPIBase& operator=(
      const BraveVPNWireguardConnectionAPIBase&) = delete;
  ~BraveVPNWireguardConnectionAPIBase() override;

  // BraveVPNOSConnectionAPI
  void FetchProfileCredentials() override;
  void SetSelectedRegion(const std::string& name) override;
  void Connect() override;

  // Platform dependent APIs.
  virtual void PlatformConnectImpl(
      const wireguard::WireguardProfileCredentials& credentials) = 0;

  // BraveVPNOSConnectionAPI::Observer
  void OnConnectionStateChanged(mojom::ConnectionState state) override;

 protected:
  void OnDisconnected(bool success);
  void RequestNewProfileCredentials(
      brave_vpn::wireguard::WireguardKeyPair key_pair);
  void OnGetProfileCredentials(const std::string& client_private_key,
                               const std::string& profile_credential,
                               bool success);
  void OnVerifyCredentials(const std::string& result, bool success);

 private:
  void ResetConnectionInfo();

  base::WeakPtrFactory<BraveVPNWireguardConnectionAPIBase> weak_factory_{this};
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_BRAVE_VPN_WIREGUARD_CONNECTION_API_BASE_H_
