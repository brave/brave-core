/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_CONNECTION_MANAGER_WIN_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_CONNECTION_MANAGER_WIN_H_

#include <string>

#include "base/no_destructor.h"
#include "brave/components/brave_vpn/brave_vpn_connection_manager.h"

namespace brave_vpn {

class BraveVPNConnectionManagerWin : public BraveVPNConnectionManager {
 public:
  BraveVPNConnectionManagerWin(const BraveVPNConnectionManagerWin&) = delete;
  BraveVPNConnectionManagerWin& operator=(const BraveVPNConnectionManagerWin&) =
      delete;

 protected:
  friend class base::NoDestructor<BraveVPNConnectionManagerWin>;

  BraveVPNConnectionManagerWin();
  ~BraveVPNConnectionManagerWin() override;

  // BraveVPNConnectionManager overrides:
  bool CreateVPNConnection(const BraveVPNConnectionInfo& info) override;
  bool UpdateVPNConnection(const BraveVPNConnectionInfo& info) override;
  bool Connect(const std::string& name) override;
  bool Disconnect(const std::string& name) override;
  bool RemoveVPNConnection(const std::string& name) override;
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_CONNECTION_MANAGER_WIN_H_
