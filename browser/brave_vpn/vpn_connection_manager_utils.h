/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_VPN_CONNECTION_MANAGER_UTILS_H_
#define BRAVE_BROWSER_BRAVE_VPN_VPN_CONNECTION_MANAGER_UTILS_H_

#include <memory>

#include "base/memory/scoped_refptr.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

class PrefService;

namespace brave_vpn {

class BraveVPNConnectionManager;

std::unique_ptr<BraveVPNConnectionManager> CreateBraveVPNConnectionManager(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs);

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_VPN_CONNECTION_MANAGER_UTILS_H_
