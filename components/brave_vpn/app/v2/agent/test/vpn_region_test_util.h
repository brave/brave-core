/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_VPN_REGION_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_VPN_REGION_TEST_UTIL_H_

#include <optional>
#include <string>

#include "brave/components/brave_vpn/common/mojom/browser_agent.mojom-forward.h"

namespace brave_vpn::v2 {

// Returns a country-level region with only |name| set. Tests set any other
// fields they need on the result.
mojom::VpnRegionPtr MakeTestRegion(std::string name);

// Returns a selection the user made explicitly.
mojom::VpnRegionSelectionPtr MakeTestPickedSelection(std::string name);

// Returns an automatic selection, resolved to |name| or unresolved if it's
// null.
mojom::VpnRegionSelectionPtr MakeTestAutomaticSelection(
    std::optional<std::string> name);

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_VPN_REGION_TEST_UTIL_H_
