/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_CONNECTION_MAC_BRAVE_VPN_OS_CONNECTION_API_MAC_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_CONNECTION_MAC_BRAVE_VPN_OS_CONNECTION_API_MAC_H_

#import <Foundation/Foundation.h>
#include <string>

#include "base/no_destructor.h"
#include "brave/components/brave_vpn/connection/brave_vpn_os_connection_api_base.h"

namespace brave_vpn {

class BraveVPNOSConnectionAPIMac : public BraveVPNOSConnectionAPIBase {
 public:
  BraveVPNOSConnectionAPIMac(const BraveVPNOSConnectionAPIMac&) = delete;
  BraveVPNOSConnectionAPIMac& operator=(const BraveVPNOSConnectionAPIMac&) =
      delete;

 protected:
  friend class base::NoDestructor<BraveVPNOSConnectionAPIMac>;

  BraveVPNOSConnectionAPIMac();
  ~BraveVPNOSConnectionAPIMac() override;

 private:
  // BraveVPNOSConnectionAPIBase overrides:
  void CreateVPNConnectionImpl(const BraveVPNConnectionInfo& info) override;
  void RemoveVPNConnectionImpl(const std::string& name) override;
  void ConnectImpl(const std::string& name) override;
  void DisconnectImpl(const std::string& name) override;
  void CheckConnectionImpl(const std::string& name) override;
  void ObserveVPNConnectionChange();

  id vpn_observer_ = nil;
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_CONNECTION_MAC_BRAVE_VPN_OS_CONNECTION_API_MAC_H_
