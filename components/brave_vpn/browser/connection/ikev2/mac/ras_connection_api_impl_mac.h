/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_MAC_RAS_CONNECTION_API_IMPL_MAC_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_MAC_RAS_CONNECTION_API_IMPL_MAC_H_

#include <string>

#include "brave/components/brave_vpn/browser/connection/ikev2/ras_connection_api_impl_base.h"

namespace brave_vpn {

class RasConnectionAPIImplMac : public RasConnectionAPIImplBase {
 public:
  RasConnectionAPIImplMac(
      BraveVPNOSConnectionAPI* api,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~RasConnectionAPIImplMac() override;

 private:
  // RasConnectionAPIImplBase overrides:
  void CreateVPNConnectionImpl(const BraveVPNConnectionInfo& info) override;
  void ConnectImpl(const std::string& name) override;
  void DisconnectImpl(const std::string& name) override;
  void CheckConnectionImpl(const std::string& name) override;
  bool IsPlatformNetworkAvailable() override;

  void ObserveVPNConnectionChange();

  id vpn_observer_ = nil;
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_MAC_RAS_CONNECTION_API_IMPL_MAC_H_
