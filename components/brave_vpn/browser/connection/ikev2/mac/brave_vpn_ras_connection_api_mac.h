/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_MAC_BRAVE_VPN_RAS_CONNECTION_API_MAC_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_MAC_BRAVE_VPN_RAS_CONNECTION_API_MAC_H_

#include <string>

#include "brave/components/brave_vpn/browser/connection/ikev2/brave_vpn_ras_connection_api_base.h"

namespace brave_vpn {

class BraveVPNOSConnectionAPIMac : public BraveVPNOSConnectionAPIBase {
 public:
  BraveVPNOSConnectionAPIMac(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* local_prefs,
      version_info::Channel channel);
  BraveVPNOSConnectionAPIMac(const BraveVPNOSConnectionAPIMac&) = delete;
  BraveVPNOSConnectionAPIMac& operator=(const BraveVPNOSConnectionAPIMac&) =
      delete;
  ~BraveVPNOSConnectionAPIMac() override;

 private:
  // BraveVPNOSConnectionAPIBase overrides:
  void CreateVPNConnectionImpl(const BraveVPNConnectionInfo& info) override;
  void ConnectImpl(const std::string& name) override;
  void DisconnectImpl(const std::string& name) override;
  void CheckConnectionImpl(const std::string& name) override;
  bool IsPlatformNetworkAvailable() override;
  void ObserveVPNConnectionChange();

  id vpn_observer_ = nil;
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_MAC_BRAVE_VPN_RAS_CONNECTION_API_MAC_H_
