/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_CONNECTION_API_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_CONNECTION_API_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_vpn/browser/api/brave_vpn_api_request.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_os_connection_api.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/wireguard_utils.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

class PrefService;

namespace brave_vpn {

class BraveVPNWireguardConnectionAPI : public BraveVPNOSConnectionAPI {
 public:
  BraveVPNWireguardConnectionAPI(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* local_prefs,
      version_info::Channel channel);

  BraveVPNWireguardConnectionAPI(const BraveVPNWireguardConnectionAPI&) =
      delete;
  BraveVPNWireguardConnectionAPI& operator=(
      const BraveVPNWireguardConnectionAPI&) = delete;
  ~BraveVPNWireguardConnectionAPI() override;

  void FetchProfileCredentials() override;

  // BraveVPNOSConnectionAPI
  void Connect() override;
  void Disconnect() override;
  void CheckConnection() override;
  void SetSelectedRegion(const std::string& name) override;

  std::string GetCurrentEnvironment() const;

 private:
  void OnWireguardServiceLaunched(bool success);
  void OnGetProfileCredentials(const std::string& client_private_key,
                               const std::string& profile_credential,
                               bool success);
  void OnWireguardKeypairGenerated(
      brave_vpn::internal::WireguardKeyPair key_pair);
  void OnDisconnected(bool success);

  raw_ptr<PrefService> local_prefs_ = nullptr;
  // Only not null when there is active network request.
  // When network request is done, we reset this so we can know
  // whether we're waiting the response or not.
  // We can cancel connecting request quickly when fetching hostnames or
  // profile credentials is not yet finished by reset this.
  std::unique_ptr<BraveVpnAPIRequest> api_request_;

  base::WeakPtrFactory<BraveVPNWireguardConnectionAPI> weak_factory_{this};
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_CONNECTION_API_H_
