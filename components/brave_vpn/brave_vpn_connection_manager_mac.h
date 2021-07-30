/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_CONNECTION_MANAGER_MAC_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_CONNECTION_MANAGER_MAC_H_

#import <NetworkExtension/NetworkExtension.h>
#include <string>

#include "base/no_destructor.h"
#include "brave/components/brave_vpn/brave_vpn_connection_manager.h"

namespace brave_vpn {

class BraveVPNConnectionManagerMac : public BraveVPNConnectionManager {
 public:
  BraveVPNConnectionManagerMac(const BraveVPNConnectionManagerMac&) = delete;
  BraveVPNConnectionManagerMac& operator=(const BraveVPNConnectionManagerMac&) =
      delete;

 protected:
  friend class base::NoDestructor<BraveVPNConnectionManagerMac>;

  BraveVPNConnectionManagerMac();
  ~BraveVPNConnectionManagerMac() override;

 private:
  // BraveVPNConnectionManager overrides:
  void CreateVPNConnection(const BraveVPNConnectionInfo& info) override;
  void UpdateVPNConnection(const BraveVPNConnectionInfo& info) override;
  void RemoveVPNConnection(const std::string& name) override;
  void Connect(const std::string& name) override;
  void Disconnect(const std::string& name) override;

  BraveVPNConnectionInfo info_;
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_CONNECTION_MANAGER_MAC_H_
