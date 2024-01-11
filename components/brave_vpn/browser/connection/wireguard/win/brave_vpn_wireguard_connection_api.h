/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_CONNECTION_API_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_CONNECTION_API_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/brave_vpn_wireguard_connection_api_base.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/credentials/brave_vpn_wireguard_profile_credentials.h"
#include "brave/components/brave_vpn/common/win/brave_windows_service_watcher.h"
#include "components/version_info/channel.h"

class PrefService;

namespace brave_vpn {

class BraveVPNWireguardConnectionAPI
    : public BraveVPNWireguardConnectionAPIBase {
 public:
  BraveVPNWireguardConnectionAPI(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* local_prefs,
      version_info::Channel channel,
      base::RepeatingCallback<bool()> service_installer);

  BraveVPNWireguardConnectionAPI(const BraveVPNWireguardConnectionAPI&) =
      delete;
  BraveVPNWireguardConnectionAPI& operator=(
      const BraveVPNWireguardConnectionAPI&) = delete;
  ~BraveVPNWireguardConnectionAPI() override;

  // BraveVPNOSConnectionAPI
  void Disconnect() override;
  void CheckConnection() override;

  // BraveVPNOSConnectionAPI::Observer
  void OnConnectionStateChanged(mojom::ConnectionState state) override;

 protected:
  // BraveVPNWireguardConnectionAPIBase
  void PlatformConnectImpl(
      const wireguard::WireguardProfileCredentials& credentials) override;

 private:
  void RunServiceWatcher();
  void OnWireguardServiceLaunched(bool success);
  void OnServiceStopped(int mask);
  void ResetServiceWatcher();

  std::unique_ptr<brave::ServiceWatcher> service_watcher_;
  version_info::Channel channel_ = version_info::Channel::UNKNOWN;
  base::WeakPtrFactory<BraveVPNWireguardConnectionAPI> weak_factory_{this};
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_CONNECTION_API_H_
