/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_MAC_KEYCHAIN_UTILS_MAC_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_MAC_KEYCHAIN_UTILS_MAC_H_

#include <CoreFoundation/CoreFoundation.h>
#include <string>

namespace brave_vpn {
NSData* CreatePersistentReference(const std::string& config,
                                  const std::string& label,
                                  const std::string& account,
                                  const std::string& description);
bool VerifyPersistentRefData(NSData* data);
bool DeleteReference(NSData* data);
}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_MAC_KEYCHAIN_UTILS_MAC_H_
