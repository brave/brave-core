/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_service_helper.h"

#include <algorithm>

#include "base/base64.h"
#include "base/notreached.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_vpn/brave_vpn_constants.h"
#include "brave/components/brave_vpn/brave_vpn_data_types.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

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
  if (auto* continent = value.FindString(kRegionContinentKey))
    region.continent = *continent;
  if (auto* name = value.FindString(kRegionNameKey))
    region.name = *name;
  if (auto* name_pretty = value.FindString(kRegionNamePrettyKey))
    region.name_pretty = *name_pretty;
  if (auto* country_iso_code = value.FindString(kRegionCountryIsoCodeKey))
    region.country_iso_code = *country_iso_code;
  return region;
}

}  // namespace

// On desktop, the environment is tied to SKUs because you would purchase it
// from `account.brave.com` (or similar, based on env). The credentials for VPN
// will always be in the same environment as the SKU environment.
//
// When the vendor receives a credential from us during auth, it also includes
// the environment. The vendor then can do a lookup using Payment Service.
std::string GetBraveVPNPaymentsEnv(const std::string& env) {
  if (env == skus::kEnvProduction)
    return "";
  // Use same value.
  if (env == skus::kEnvStaging || env == skus::kEnvDevelopment)
    return env;

  NOTREACHED();

#if defined(OFFICIAL_BUILD)
  return "";
#else
  return "development";
#endif
}

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

std::unique_ptr<Hostname> PickBestHostname(
    const std::vector<Hostname>& hostnames) {
  std::vector<Hostname> filtered_hostnames;
  std::copy_if(hostnames.begin(), hostnames.end(),
               std::back_inserter(filtered_hostnames),
               [](const Hostname& hostname) { return !hostname.is_offline; });

  std::sort(filtered_hostnames.begin(), filtered_hostnames.end(),
            [](const Hostname& a, const Hostname& b) {
              return a.capacity_score > b.capacity_score;
            });

  if (filtered_hostnames.empty())
    return std::make_unique<Hostname>();

  // Pick highest capacity score.
  return std::make_unique<Hostname>(filtered_hostnames[0]);
}

std::vector<Hostname> ParseHostnames(const base::Value::List& hostnames_value) {
  std::vector<Hostname> hostnames;
  for (const auto& value : hostnames_value) {
    DCHECK(value.is_dict());
    if (!value.is_dict())
      continue;

    const auto& dict = value.GetDict();
    constexpr char kHostnameKey[] = "hostname";
    constexpr char kDisplayNameKey[] = "display-name";
    constexpr char kOfflineKey[] = "offline";
    constexpr char kCapacityScoreKey[] = "capacity-score";
    const std::string* hostname_str = dict.FindString(kHostnameKey);
    const std::string* display_name_str = dict.FindString(kDisplayNameKey);
    absl::optional<bool> offline = dict.FindBool(kOfflineKey);
    absl::optional<int> capacity_score = dict.FindInt(kCapacityScoreKey);

    if (!hostname_str || !display_name_str || !offline || !capacity_score)
      continue;

    hostnames.push_back(
        Hostname{*hostname_str, *display_name_str, *offline, *capacity_score});
  }

  return hostnames;
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

base::Value::Dict GetValueWithTicketInfos(const std::string& email,
                                          const std::string& subject,
                                          const std::string& body) {
  base::Value::Dict dict;

  std::string email_trimmed, subject_trimmed, body_trimmed, body_encoded;
  base::TrimWhitespaceASCII(email, base::TRIM_ALL, &email_trimmed);
  base::TrimWhitespaceASCII(subject, base::TRIM_ALL, &subject_trimmed);
  base::TrimWhitespaceASCII(body, base::TRIM_ALL, &body_trimmed);
  base::Base64Encode(body_trimmed, &body_encoded);

  // required fields
  dict.Set("email", email_trimmed);
  dict.Set("subject", subject_trimmed);
  dict.Set("support-ticket", body_encoded);
  dict.Set("partner-client-id", "com.brave.browser");

  // optional (but encouraged) fields
  dict.Set("subscriber-credential", "");
  dict.Set("payment-validation-method", "brave-premium");
  dict.Set("payment-validation-data", "");

  return dict;
}

mojom::RegionPtr GetRegionPtrWithNameFromRegionList(
    const std::string& name,
    const std::vector<mojom::Region> region_list) {
  auto it = base::ranges::find(region_list, name, &mojom::Region::name);
  if (it != region_list.end())
    return it->Clone();
  return mojom::RegionPtr();
}

}  // namespace brave_vpn
