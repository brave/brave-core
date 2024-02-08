/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_VPN_UTILS_H_
#define BRAVE_BROWSER_BRAVE_VPN_VPN_UTILS_H_

#include <memory>

#include "base/functional/callback_forward.h"
#include "base/memory/scoped_refptr.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

class PrefService;

namespace brave_vpn {

class BraveVPNOSConnectionAPI;

bool IsBraveVPNEnabled(content::BrowserContext* context);
bool IsAllowedForContext(content::BrowserContext* context);
std::unique_ptr<BraveVPNOSConnectionAPI> CreateBraveVPNConnectionAPI(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs,
    base::RepeatingCallback<bool()> service_installer);

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_VPN_UTILS_H_
