/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_service_helper.h"

#include <algorithm>
#include <utility>

#include "base/base64.h"
#include "base/json/values_util.h"
#include "base/notreached.h"
#include "base/ranges/algorithm.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "brave/components/brave_vpn/common/brave_vpn_data_types.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "components/prefs/pref_service.h"

namespace brave_vpn {

namespace {

bool IsValidRegionValue(const base::Value::Dict& value) {
  if (!value.FindString(kRegionContinentKey) ||
      !value.FindString(kRegionNameKey) ||
      !value.FindString(kRegionNamePrettyKey) ||
      !value.FindString(kRegionCountryIsoCodeKey)) {
    return false;
  }

  return true;
}

mojom::Region GetRegionFromValue(const base::Value::Dict& value) {
  mojom::Region region;
  if (auto* continent = value.FindString(brave_vpn::kRegionContinentKey))
    region.continent = *continent;
  if (auto* name = value.FindString(brave_vpn::kRegionNameKey))
    region.name = *name;
  if (auto* name_pretty = value.FindString(brave_vpn::kRegionNamePrettyKey))
    region.name_pretty = *name_pretty;
  if (auto* country_iso_code =
          value.FindString(brave_vpn::kRegionCountryIsoCodeKey))
    region.country_iso_code = *country_iso_code;
  return region;
}

}  // namespace

bool ValidateCachedRegionData(const base::Value::List& region_value) {
  for (const auto& value : region_value) {
    // Make sure cached one has all latest properties.
    if (!value.is_dict() || !IsValidRegionValue(value.GetDict()))
      return false;
  }

  return true;
}

base::Value::Dict GetValueFromRegion(const mojom::Region& region) {
  base::Value::Dict region_dict;
  region_dict.Set(kRegionContinentKey, region.continent);
  region_dict.Set(kRegionNameKey, region.name);
  region_dict.Set(kRegionNamePrettyKey, region.name_pretty);
  region_dict.Set(kRegionCountryIsoCodeKey, region.country_iso_code);
  return region_dict;
}

std::vector<mojom::Region> ParseRegionList(
    const base::Value::List& region_list) {
  std::vector<mojom::Region> regions;
  for (const auto& value : region_list) {
    DCHECK(value.is_dict());
    if (!value.is_dict())
      continue;
    regions.push_back(GetRegionFromValue(value.GetDict()));
  }

  // Sort region list alphabetically
  std::sort(regions.begin(), regions.end(),
            [](mojom::Region& a, mojom::Region& b) {
              return (a.name_pretty < b.name_pretty);
            });
  return regions;
}

mojom::RegionPtr GetRegionPtrWithNameFromRegionList(
    const std::string& name,
    const std::vector<mojom::Region> region_list) {
  auto it = base::ranges::find(region_list, name, &mojom::Region::name);
  if (it != region_list.end())
    return it->Clone();
  return mojom::RegionPtr();
}

bool IsValidCredentialSummary(const base::Value& summary) {
  DCHECK(summary.is_dict());
  const bool active = summary.GetDict().FindBool("active").value_or(false);
  const int remaining_credential_count =
      summary.GetDict().FindInt("remaining_credential_count").value_or(0);
  return active && remaining_credential_count > 0;
}

bool HasSubscriberCredential(PrefService* local_prefs) {
  const base::Value::Dict& sub_cred_dict =
      local_prefs->GetDict(prefs::kBraveVPNSubscriberCredential);
  return !sub_cred_dict.empty();
}

absl::optional<base::Time> GetExpirationTime(PrefService* local_prefs) {
  if (!HasValidSubscriberCredential(local_prefs))
    return absl::nullopt;

  const base::Value::Dict& sub_cred_dict =
      local_prefs->GetDict(prefs::kBraveVPNSubscriberCredential);

  const base::Value* expiration_time_value =
      sub_cred_dict.Find(kSubscriberCredentialExpirationKey);

  if (!expiration_time_value)
    return absl::nullopt;

  return base::ValueToTime(expiration_time_value);
}

void SetSubscriberCredential(PrefService* local_prefs,
                             const std::string& subscriber_credential,
                             const base::Time& expiration_time) {
  base::Value::Dict cred_dict;
  cred_dict.Set(kSubscriberCredentialKey, subscriber_credential);
  cred_dict.Set(kSubscriberCredentialExpirationKey,
                base::TimeToValue(expiration_time));
  local_prefs->SetDict(prefs::kBraveVPNSubscriberCredential,
                       std::move(cred_dict));
}

void ClearSubscriberCredential(PrefService* local_prefs) {
  local_prefs->ClearPref(prefs::kBraveVPNSubscriberCredential);
}

}  // namespace brave_vpn
