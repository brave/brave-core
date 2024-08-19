/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/brave_vpn_region_data_helper.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "base/base64.h"
#include "base/json/values_util.h"
#include "base/notreached.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "brave/components/brave_vpn/common/brave_vpn_data_types.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "components/prefs/pref_service.h"
#include "third_party/icu/source/i18n/unicode/timezone.h"

namespace brave_vpn {
mojom::RegionPtr GetRegionPtrWithNameFromRegionList(
    const std::string& name,
    const std::vector<mojom::RegionPtr>& region_list) {
  auto it = base::ranges::find(region_list, name, &mojom::Region::name);
  if (it != region_list.end()) {
    return it->Clone();
  }
  return mojom::RegionPtr();
}

base::Value::Dict GetValueFromRegionWithoutCity(
    const mojom::RegionPtr& region) {
  base::Value::Dict region_dict;
  region_dict.Set(kRegionNameKey, region->name);
  region_dict.Set(kRegionNamePrettyKey, region->name_pretty);
  region_dict.Set(kRegionContinentKey, region->continent);
  region_dict.Set(kRegionCountryIsoCodeKey, region->country_iso_code);
  region_dict.Set(kRegionPrecisionKey, region->region_precision);
  region_dict.Set(kRegionLatitudeKey, region->latitude);
  region_dict.Set(kRegionLongitudeKey, region->longitude);
  region_dict.Set(kRegionServerCountKey, region->server_count);
  return region_dict;
}

base::Value::Dict GetValueFromRegion(const mojom::RegionPtr& region) {
  base::Value::Dict region_dict = GetValueFromRegionWithoutCity(region);
  if (!region->cities.empty()) {
    base::Value::List cities;
    for (const auto& city : region->cities) {
      cities.Append(GetValueFromRegionWithoutCity(city));
    }
    region_dict.Set(kRegionCitiesKey, std::move(cities));
  }
  return region_dict;
}

bool IsValidRegionValue(const base::Value::Dict& value) {
  if (!value.FindString(kRegionNameKey) ||
      !value.FindString(kRegionNamePrettyKey) ||
      !value.FindString(kRegionContinentKey) ||
      !value.FindString(kRegionCountryIsoCodeKey) ||
      !value.FindString(kRegionPrecisionKey) ||
      !value.FindList(kRegionCitiesKey) ||
      !value.FindString(kRegionLatitudeKey) ||
      !value.FindString(kRegionLongitudeKey) ||
      !value.FindString(kRegionServerCountKey)) {
    return false;
  }

  return true;
}

mojom::RegionPtr GetRegionFromValueWithoutCity(const base::Value::Dict& value) {
  mojom::RegionPtr region = mojom::Region::New();
  if (auto* name = value.FindString(brave_vpn::kRegionNameKey)) {
    region->name = *name;
  }
  if (auto* name_pretty = value.FindString(brave_vpn::kRegionNamePrettyKey)) {
    region->name_pretty = *name_pretty;
  }
  if (auto* continent = value.FindString(brave_vpn::kRegionContinentKey)) {
    region->continent = *continent;
  }
  if (auto* country_iso_code =
          value.FindString(brave_vpn::kRegionCountryIsoCodeKey)) {
    region->country_iso_code = *country_iso_code;
  }
  if (auto* region_precision =
          value.FindString(brave_vpn::kRegionPrecisionKey)) {
    region->region_precision = *region_precision;
  }
  if (auto latitude = value.FindDouble(brave_vpn::kRegionLatitudeKey)) {
    region->latitude = *latitude;
  }
  if (auto longitude = value.FindDouble(brave_vpn::kRegionLongitudeKey)) {
    region->longitude = *longitude;
  }
  if (auto server_count = value.FindInt(brave_vpn::kRegionServerCountKey)) {
    region->server_count = *server_count;
  }

  return region;
}

mojom::RegionPtr GetRegionFromValue(const base::Value::Dict& value) {
  mojom::RegionPtr region = GetRegionFromValueWithoutCity(value);
  if (value.FindList(kRegionCitiesKey)) {
    const auto* cities = value.FindList(kRegionCitiesKey);
    for (const auto& city : *cities) {
      region->cities.push_back(GetRegionFromValueWithoutCity(city.GetDict()));
    }
  }

  return region;
}

bool ValidateCachedRegionData(const base::Value::List& region_value) {
  for (const auto& value : region_value) {
    // Make sure cached one has all latest properties.
    if (!value.is_dict() || !IsValidRegionValue(value.GetDict())) {
      return false;
    }
  }

  return true;
}

std::vector<mojom::RegionPtr> ParseRegionList(
    const base::Value::List& region_list) {
  std::vector<mojom::RegionPtr> regions;
  for (const auto& value : region_list) {
    DCHECK(value.is_dict());
    if (!value.is_dict()) {
      continue;
    }

    regions.push_back(GetRegionFromValue(value.GetDict()).Clone());
  }

  // Sort region list alphabetically
  std::sort(regions.begin(), regions.end(),
            [](mojom::RegionPtr& a, mojom::RegionPtr& b) {
              return (a->name_pretty < b->name_pretty);
            });
  return regions;
}

}  // namespace brave_vpn
