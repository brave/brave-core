/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_MAC_BRAVE_VPN_RAS_HELPER_BRAVE_VPN_CONNECTION_UTILS_MAC_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_MAC_BRAVE_VPN_RAS_HELPER_BRAVE_VPN_CONNECTION_UTILS_MAC_H_

#include <string>

namespace brave {
int CreateVPN(const std::string& connection_name);
}

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_MAC_BRAVE_VPN_RAS_HELPER_BRAVE_VPN_CONNECTION_UTILS_MAC_H_
