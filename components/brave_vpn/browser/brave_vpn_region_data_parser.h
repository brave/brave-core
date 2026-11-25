/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_BRAVE_VPN_REGION_DATA_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_BRAVE_VPN_REGION_DATA_PARSER_H_

#include <vector>

#include "base/values.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom-forward.h"

namespace brave_vpn {

// JSON keys of a region entry in the server response.
inline constexpr char kRegionNameKey[] = "name";
inline constexpr char kRegionNamePrettyKey[] = "name-pretty";
inline constexpr char kRegionCountryKey[] = "country";
inline constexpr char kRegionContinentKey[] = "continent";
inline constexpr char kRegionCountryIsoCodeKey[] = "country-iso-code";
inline constexpr char kRegionPrecisionKey[] = "region-precision";
inline constexpr char kRegionCitiesKey[] = "cities";
inline constexpr char kRegionServerCountKey[] = "server-count";
inline constexpr char kRegionSmartRoutingProxyStateKey[] =
    "smart-routing-proxy-state";

// Parses the region list returned by the VPN server's regions endpoint.
// Non-dictionary entries are skipped; the result is sorted by |name_pretty|.
std::vector<mojom::RegionPtr> ParseRegionList(
    const base::ListValue& region_list);

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_BRAVE_VPN_REGION_DATA_PARSER_H_
