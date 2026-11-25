/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_REGION_CONVERSIONS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_REGION_CONVERSIONS_H_

#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom-forward.h"
#include "brave/components/brave_vpn/common/mojom/browser_agent.mojom-forward.h"

namespace brave_vpn::v2 {

// Converts a region from the agent representation to the browser
// representation, since in V2, regions are owned by the agent and the browser
// only maintains a view of them.
mojom::RegionPtr BrowserRegionFromAgent(mojom::VpnRegionPtr agent_region);

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_REGION_CONVERSIONS_H_
