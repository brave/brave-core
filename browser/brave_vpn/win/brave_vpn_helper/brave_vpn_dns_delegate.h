// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_HELPER_BRAVE_VPN_DNS_DELEGATE_H_
#define BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_HELPER_BRAVE_VPN_DNS_DELEGATE_H_

namespace brave_vpn {

class BraveVpnDnsDelegate {
 public:
  virtual void SignalExit() = 0;
};

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_HELPER_BRAVE_VPN_DNS_DELEGATE_H_
