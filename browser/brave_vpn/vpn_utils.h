/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_VPN_UTILS_H_
#define BRAVE_BROWSER_BRAVE_VPN_VPN_UTILS_H_

namespace content {
class BrowserContext;
}  // namespace content

namespace brave_vpn {

bool IsBraveVPNEnabled(content::BrowserContext* context);
bool IsAllowedForContext(content::BrowserContext* context);

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_VPN_UTILS_H_
