/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_SWITCHES_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_SWITCHES_H_

namespace brave_vpn {

namespace switches {

// Value should be "connection-name:host-name:user-name:password".
constexpr char kBraveVPNTestCredentials[] = "brave-vpn-test-credentials";
// Use for simulation instead of calling os platform apis.
constexpr char kBraveVPNSimulation[] = "brave-vpn-simulate";

}  // namespace switches

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_SWITCHES_H_
