/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_OS_CONNECTION_API_MAC_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_OS_CONNECTION_API_MAC_H_

#include <string>

#include "base/no_destructor.h"
#include "brave/components/brave_vpn/brave_vpn_os_connection_api.h"

namespace brave_vpn {

class BraveVPNOSConnectionAPIMac : public BraveVPNOSConnectionAPI {
 public:
  BraveVPNOSConnectionAPIMac(const BraveVPNOSConnectionAPIMac&) = delete;
  BraveVPNOSConnectionAPIMac& operator=(const BraveVPNOSConnectionAPIMac&) =
      delete;

 protected:
  friend class base::NoDestructor<BraveVPNOSConnectionAPIMac>;

  BraveVPNOSConnectionAPIMac();
  ~BraveVPNOSConnectionAPIMac() override;

 private:
  // BraveVPNOSConnectionAPI overrides:
  void CreateVPNConnection(const BraveVPNConnectionInfo& info) override;
  void UpdateVPNConnection(const BraveVPNConnectionInfo& info) override;
  void RemoveVPNConnection(const std::string& name) override;
  void Connect(const std::string& name) override;
  void Disconnect(const std::string& name) override;
  void CheckConnection(const std::string& name) override;
  void ObserveVPNConnectionChange();

  id vpn_observer_ = nil;
  BraveVPNConnectionInfo info_;
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_OS_CONNECTION_API_MAC_H_
