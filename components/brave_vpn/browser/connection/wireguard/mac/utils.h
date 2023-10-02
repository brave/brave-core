/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_MAC_BRAVE_VPN_WIREGUARD_BRIDGE_UTILS_H_
#define BRAVE_BROWSER_BRAVE_VPN_MAC_BRAVE_VPN_WIREGUARD_BRIDGE_UTILS_H_

#include <string>
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_vpn {
absl::optional<std::string> GetConfigStringValue(const std::string& name,
                                                 const std::string& config);

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_MAC_BRAVE_VPN_WIREGUARD_BRIDGE_UTILS_H_
