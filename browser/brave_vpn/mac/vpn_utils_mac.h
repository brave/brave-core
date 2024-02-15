// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_VPN_MAC_VPN_UTILS_MAC_H_
#define BRAVE_BROWSER_BRAVE_VPN_MAC_VPN_UTILS_MAC_H_

#include <memory>

#include "base/memory/scoped_refptr.h"
#include "brave/components/brave_vpn/browser/connection/connection_api_impl.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_vpn {

class BraveVPNConnectionManager;

std::unique_ptr<ConnectionAPIImpl> CreateConnectionAPIImplMac(
    BraveVPNConnectionManager* manager,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_MAC_VPN_UTILS_MAC_H_
