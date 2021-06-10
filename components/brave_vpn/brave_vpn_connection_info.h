/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_CONNECTION_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_CONNECTION_INFO_H_

#include <string>

namespace brave_vpn {

struct BraveVPNConnectionInfo {
  std::string connection_name;
  std::string hostname;
  std::string username;
  std::string password;

  BraveVPNConnectionInfo();
  ~BraveVPNConnectionInfo();
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_CONNECTION_INFO_H_
