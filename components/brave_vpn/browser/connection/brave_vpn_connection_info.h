/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_BRAVE_VPN_CONNECTION_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_BRAVE_VPN_CONNECTION_INFO_H_

#include <string>

namespace brave_vpn {

class BraveVPNConnectionInfo {
 public:
  BraveVPNConnectionInfo();
  ~BraveVPNConnectionInfo();
  BraveVPNConnectionInfo(const BraveVPNConnectionInfo& info);
  BraveVPNConnectionInfo& operator=(const BraveVPNConnectionInfo& info);

  void Reset();
  bool IsValid() const;
  void SetConnectionInfo(const std::string& connection_name,
                         const std::string& hostname,
                         const std::string& username,
                         const std::string& password,
                         bool smart_routing_enabled,
                         const std::string& proxy);

  std::string connection_name() const { return connection_name_; }
  std::string hostname() const { return hostname_; }
  std::string username() const { return username_; }
  std::string password() const { return password_; }
  bool smart_routing_enabled() const { return smart_routing_enabled_; }
  std::string proxy() const { return proxy_; }
  void set_smart_routing_enabled(bool enabled) {
    smart_routing_enabled_ = enabled;
  }

 private:
  std::string connection_name_;
  std::string hostname_;
  std::string username_;
  std::string password_;
  bool smart_routing_enabled_;
  std::string proxy_;
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_BRAVE_VPN_CONNECTION_INFO_H_
