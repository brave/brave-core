/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_MAC_BRAVE_VPN_WIREGUARD_BRIDGE_BRAVE_VPN_RUNNER_MAC_H_
#define BRAVE_BROWSER_BRAVE_VPN_MAC_BRAVE_VPN_WIREGUARD_BRIDGE_BRAVE_VPN_RUNNER_MAC_H_

#include <string>

#include "base/functional/callback.h"
#include "base/no_destructor.h"

namespace brave_vpn {

class BraveVpnRunnerMac {
 public:
  static BraveVpnRunnerMac* GetInstance();

  BraveVpnRunnerMac(const BraveVpnRunnerMac&) = delete;
  BraveVpnRunnerMac& operator=(const BraveVpnRunnerMac&) = delete;

  int EnableVPN(const std::string& config);
  int RemoveVPN();

 private:
  friend class base::NoDestructor<BraveVpnRunnerMac>;

  BraveVpnRunnerMac();
  ~BraveVpnRunnerMac();

  int RunLoop();
  void SignalExit(int code, const std::string& message);

  void CreateTunnelProvider(const std::string& config);

  base::OnceClosure quit_;
};

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_MAC_BRAVE_VPN_WIREGUARD_BRIDGE_BRAVE_VPN_RUNNER_MAC_H_
