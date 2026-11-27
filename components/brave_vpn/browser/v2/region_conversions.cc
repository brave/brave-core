/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/region_conversions.h"

#include <string>
#include <utility>

#include "base/check.h"
#include "base/notreached.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "brave/components/brave_vpn/common/mojom/browser_agent.mojom.h"

namespace brave_vpn::v2 {
namespace {

std::string RegionPrecisionToString(mojom::RegionPrecision precision) {
  switch (precision) {
    case mojom::RegionPrecision::kDefault:
      return mojom::kRegionPrecisionDefault;
    case mojom::RegionPrecision::kCity:
      return mojom::kRegionPrecisionCity;
    case mojom::RegionPrecision::kCountry:
      return mojom::kRegionPrecisionCountry;
    case mojom::RegionPrecision::kCityByCountry:
      return mojom::kRegionPrecisionCityByCountry;
  }
  NOTREACHED();
}

std::string SmartProxyStateToString(mojom::SmartProxyState state) {
  switch (state) {
    case mojom::SmartProxyState::kAll:
      return "all";
    case mojom::SmartProxyState::kSome:
      return "some";
    case mojom::SmartProxyState::kNone:
      return "none";
  }
  NOTREACHED();
}

}  // namespace

mojom::RegionPtr BrowserRegionFromAgent(mojom::VpnRegionPtr agent_region) {
  CHECK(agent_region);

  auto region = mojom::Region::New();
  region->name = std::move(agent_region->name);
  region->name_pretty = std::move(agent_region->title);
  region->country = std::move(agent_region->country);
  region->continent = std::move(agent_region->continent);
  region->country_iso_code = std::move(agent_region->country_iso_code);
  region->server_count = agent_region->server_count;
  region->region_precision =
      RegionPrecisionToString(agent_region->region_precision);
  region->smart_routing_proxy_state =
      SmartProxyStateToString(agent_region->smart_proxy_state);

  region->cities.reserve(agent_region->subregions.size());
  for (auto& subregion : agent_region->subregions) {
    region->cities.push_back(BrowserRegionFromAgent(std::move(subregion)));
  }

  return region;
}

}  // namespace brave_vpn::v2
