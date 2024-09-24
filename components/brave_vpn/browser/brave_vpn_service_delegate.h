/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_BRAVE_VPN_SERVICE_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_BRAVE_VPN_SERVICE_DELEGATE_H_

#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"

namespace brave_vpn {

class BraveVPNServiceDelegate {
 public:
  BraveVPNServiceDelegate() = default;
  virtual ~BraveVPNServiceDelegate() = default;

  BraveVPNServiceDelegate(const BraveVPNServiceDelegate&) = delete;
  BraveVPNServiceDelegate& operator=(const BraveVPNServiceDelegate&) = delete;

  virtual void WriteConnectionState(mojom::ConnectionState state) = 0;
  virtual void ShowBraveVpnStatusTrayIcon() = 0;
  virtual void LaunchVPNPanel() = 0;
  virtual void OpenVpnUI(const std::string& type) = 0;
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_BRAVE_VPN_SERVICE_DELEGATE_H_
