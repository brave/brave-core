// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_SERVICE_DELEGATE_MAC_H_
#define BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_SERVICE_DELEGATE_MAC_H_

#include "brave/components/brave_vpn/browser/brave_vpn_service_delegate.h"

namespace brave_vpn {

class BraveVPNServiceDelegateMac : public BraveVPNServiceDelegate {
 public:
  BraveVPNServiceDelegateMac();
  ~BraveVPNServiceDelegateMac() override;

  BraveVPNServiceDelegateMac(const BraveVPNServiceDelegateMac&) = delete;
  BraveVPNServiceDelegateMac& operator=(const BraveVPNServiceDelegateMac&) =
      delete;

  void WriteConnectionState(mojom::ConnectionState state) override;
  void ShowBraveVpnStatusTrayIcon() override;
  void LaunchVPNPanel() override;
  void OpenVpnUI(const std::string& type) override;
};

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_SERVICE_DELEGATE_MAC_H_
