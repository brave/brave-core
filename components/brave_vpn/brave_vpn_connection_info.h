/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_CONNECTION_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_CONNECTION_INFO_H_

#include <string>

namespace brave_vpn {

class BraveVPNConnectionInfo {
 public:
  BraveVPNConnectionInfo();
  ~BraveVPNConnectionInfo();
  BraveVPNConnectionInfo(const BraveVPNConnectionInfo& info);
  BraveVPNConnectionInfo& operator=(const BraveVPNConnectionInfo& info);

  bool IsValid() const;
  void SetConnectionInfo(const std::string& connection_name,
                         const std::string& hostname,
                         const std::string& username,
                         const std::string& password);

  std::string connection_name() const { return connection_name_; }
  std::string hostname() const { return hostname_; }
  std::string username() const { return username_; }
  std::string password() const { return password_; }

 private:
  std::string connection_name_;
  std::string hostname_;
  std::string username_;
  std::string password_;
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_CONNECTION_INFO_H_
