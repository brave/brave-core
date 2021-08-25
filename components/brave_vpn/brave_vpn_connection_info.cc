/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_connection_info.h"

namespace brave_vpn {

BraveVPNConnectionInfo::BraveVPNConnectionInfo() = default;
BraveVPNConnectionInfo::~BraveVPNConnectionInfo() = default;

BraveVPNConnectionInfo::BraveVPNConnectionInfo(
    const BraveVPNConnectionInfo& info) = default;
BraveVPNConnectionInfo& BraveVPNConnectionInfo::operator=(
    const BraveVPNConnectionInfo& info) = default;

bool BraveVPNConnectionInfo::IsValid() const {
  // TODO(simonhong): Improve credentials validation.
  return !hostname_.empty() && !username_.empty() && !password_.empty();
}

void BraveVPNConnectionInfo::SetConnectionInfo(
    const std::string& connection_name,
    const std::string& hostname,
    const std::string& username,
    const std::string& password) {
  connection_name_ = connection_name;
  hostname_ = hostname;
  username_ = username;
  password_ = password;
}

}  // namespace brave_vpn
