/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_MAC_WIREGUARD_UTILS_MAC_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_MAC_WIREGUARD_UTILS_MAC_H_

#include <CoreFoundation/CoreFoundation.h>
#import <NetworkExtension/NetworkExtension.h>

#include <string>

#include "base/functional/callback.h"

namespace brave_vpn {

using TunnelProviderManagerCallback =
    base::OnceCallback<void(NETunnelProviderManager* tunnel_provider_manager)>;

void FindTunnelProviderManager(const std::string& entry_name,
                               TunnelProviderManagerCallback callback);
void CreateNewTunnelProviderManager(const std::string& config,
                                    const std::string& entry_name,
                                    const std::string& account_name,
                                    const std::string& entry_description,
                                    TunnelProviderManagerCallback callback);

std::string NSErrorToUTF8(NSError* error);
std::string NEVPNStatusToString(NEVPNStatus status);
}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_MAC_WIREGUARD_UTILS_MAC_H_
