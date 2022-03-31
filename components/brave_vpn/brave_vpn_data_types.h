/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_DATA_TYPES_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_DATA_TYPES_H_

#include <string>

namespace brave_vpn {

struct Hostname {
  std::string hostname;
  std::string display_name;
  bool is_offline;
  int capacity_score;
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_DATA_TYPES_H_
