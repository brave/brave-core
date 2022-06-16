/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_service_helper.h"

#include <algorithm>

#include "base/base64.h"
#include "base/notreached.h"
#include "base/values.h"
#include "brave/components/brave_vpn/brave_vpn_constants.h"
#include "brave/components/brave_vpn/brave_vpn_data_types.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_vpn {

namespace {

bool IsValidRegionValue(const base::Value& value) {
  if (!value.FindStringKey(kRegionContinentKey) ||
      !value.FindStringKey(kRegionNameKey) ||
      !value.FindStringKey(kRegionNamePrettyKey) ||
      !value.FindStringKey(kRegionCountryIsoCodeKey)) {
    return false;
  }

  return true;
}

mojom::Region GetRegionFromValue(const base::Value& value) {
  mojom::Region region;
  if (auto* continent = value.FindStringKey(kRegionContinentKey))
    region.continent = *continent;
  if (auto* name = value.FindStringKey(kRegionNameKey))
    region.name = *name;
  if (auto* name_pretty = value.FindStringKey(kRegionNamePrettyKey))
    region.name_pretty = *name_pretty;
  if (auto* country_iso_code = value.FindStringKey(kRegionCountryIsoCodeKey))
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

bool ValidateCachedRegionData(const base::Value& region_value) {
  for (const auto& value : region_value.GetList()) {
    // Make sure cached one has all latest properties.
    if (!IsValidRegionValue(value))
      return false;
  }

  return true;
}

base::Value GetValueFromRegion(const mojom::Region& region) {
  base::Value region_dict(base::Value::Type::DICTIONARY);
  region_dict.SetStringKey(kRegionContinentKey, region.continent);
  region_dict.SetStringKey(kRegionNameKey, region.name);
  region_dict.SetStringKey(kRegionNamePrettyKey, region.name_pretty);
  region_dict.SetStringKey(kRegionCountryIsoCodeKey, region.country_iso_code);
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

std::vector<Hostname> ParseHostnames(const base::Value& hostnames_value) {
  std::vector<Hostname> hostnames;
  for (const auto& value : hostnames_value.GetList()) {
    DCHECK(value.is_dict());
    if (!value.is_dict())
      continue;

    constexpr char kHostnameKey[] = "hostname";
    constexpr char kDisplayNameKey[] = "display-name";
    constexpr char kOfflineKey[] = "offline";
    constexpr char kCapacityScoreKey[] = "capacity-score";
    const std::string* hostname_str = value.FindStringKey(kHostnameKey);
    const std::string* display_name_str = value.FindStringKey(kDisplayNameKey);
    absl::optional<bool> offline = value.FindBoolKey(kOfflineKey);
    absl::optional<int> capacity_score = value.FindIntKey(kCapacityScoreKey);

    if (!hostname_str || !display_name_str || !offline || !capacity_score)
      continue;

    hostnames.push_back(
        Hostname{*hostname_str, *display_name_str, *offline, *capacity_score});
  }

  return hostnames;
}

std::vector<mojom::Region> ParseRegionList(const base::Value& region_list) {
  std::vector<mojom::Region> regions;
  for (const auto& value : region_list.GetList()) {
    DCHECK(value.is_dict());
    if (!value.is_dict())
      continue;
    regions.push_back(GetRegionFromValue(value));
  }

  // Sort region list alphabetically
  std::sort(regions.begin(), regions.end(),
            [](mojom::Region& a, mojom::Region& b) {
              return (a.name_pretty < b.name_pretty);
            });
  return regions;
}

base::Value GetValueWithTicketInfos(const std::string& email,
                                    const std::string& subject,
                                    const std::string& body) {
  base::Value dict(base::Value::Type::DICTIONARY);

  std::string email_trimmed, subject_trimmed, body_trimmed, body_encoded;
  base::TrimWhitespaceASCII(email, base::TRIM_ALL, &email_trimmed);
  base::TrimWhitespaceASCII(subject, base::TRIM_ALL, &subject_trimmed);
  base::TrimWhitespaceASCII(body, base::TRIM_ALL, &body_trimmed);
  base::Base64Encode(body_trimmed, &body_encoded);

  // required fields
  dict.SetStringKey("email", email_trimmed);
  dict.SetStringKey("subject", subject_trimmed);
  dict.SetStringKey("support-ticket", body_encoded);
  dict.SetStringKey("partner-client-id", "com.brave.browser");

  // optional (but encouraged) fields
  dict.SetStringKey("subscriber-credential", "");
  dict.SetStringKey("payment-validation-method", "brave-premium");
  dict.SetStringKey("payment-validation-data", "");

  return dict;
}

mojom::RegionPtr GetRegionPtrWithNameFromRegionList(
    const std::string& name,
    const std::vector<mojom::Region> region_list) {
  auto it =
      std::find_if(region_list.begin(), region_list.end(),
                   [&name](const auto& region) { return region.name == name; });
  if (it != region_list.end())
    return it->Clone();
  return mojom::RegionPtr();
}

}  // namespace brave_vpn
