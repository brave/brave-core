/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/brave_vpn_region_data_parser.h"

#include <algorithm>

#include "base/logging.h"
#include "base/values.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"

namespace brave_vpn {
namespace {

mojom::RegionPtr GetRegionFromValueWithoutCity(const base::DictValue& value) {
  mojom::RegionPtr region = mojom::Region::New();
  if (auto* name = value.FindString(kRegionNameKey)) {
    region->name = *name;
  }
  if (auto* name_pretty = value.FindString(kRegionNamePrettyKey)) {
    region->name_pretty = *name_pretty;
  }
  if (auto* country = value.FindString(kRegionCountryKey)) {
    region->country = *country;
  }
  if (auto* continent = value.FindString(kRegionContinentKey)) {
    region->continent = *continent;
  }
  if (auto* country_iso_code = value.FindString(kRegionCountryIsoCodeKey)) {
    region->country_iso_code = *country_iso_code;
  }
  if (auto* region_precision = value.FindString(kRegionPrecisionKey)) {
    region->region_precision = *region_precision;
  }
  if (auto server_count = value.FindInt(kRegionServerCountKey)) {
    region->server_count = *server_count;
  }
  if (auto* smart_routing_proxy_state =
          value.FindString(kRegionSmartRoutingProxyStateKey)) {
    region->smart_routing_proxy_state = *smart_routing_proxy_state;
  }

  // |is_automatic| is calculated in runtime.
  region->is_automatic = false;

  return region;
}

mojom::RegionPtr GetRegionFromValue(const base::DictValue& value) {
  mojom::RegionPtr region = GetRegionFromValueWithoutCity(value);
  if (const auto* cities = value.FindList(kRegionCitiesKey)) {
    for (const auto& city : *cities) {
      if (city.is_dict()) {
        region->cities.push_back(GetRegionFromValueWithoutCity(city.GetDict()));
      }
    }
  }
  return region;
}

}  // namespace

std::vector<mojom::RegionPtr> ParseRegionList(
    const base::ListValue& region_list) {
  std::vector<mojom::RegionPtr> regions;
  for (const auto& value : region_list) {
    if (!value.is_dict()) {
      VLOG(1) << __func__ << ": skipping non-dict value " << value;
      continue;
    }
    regions.push_back(GetRegionFromValue(value.GetDict()));
  }

  // Sort region list alphabetically.
  std::ranges::sort(regions, {}, &mojom::Region::name_pretty);
  return regions;
}

}  // namespace brave_vpn
