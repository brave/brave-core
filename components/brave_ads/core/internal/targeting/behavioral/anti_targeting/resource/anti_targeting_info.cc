/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_info.h"

#include <utility>

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/anti_targeting_feature.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace brave_ads {

AntiTargetingInfo::AntiTargetingInfo() = default;

AntiTargetingInfo::AntiTargetingInfo(AntiTargetingInfo&& other) noexcept =
    default;

AntiTargetingInfo& AntiTargetingInfo::operator=(
    AntiTargetingInfo&& other) noexcept = default;

AntiTargetingInfo::~AntiTargetingInfo() = default;

// static
base::expected<AntiTargetingInfo, std::string>
AntiTargetingInfo::CreateFromValue(const base::Value::Dict dict) {
  AntiTargetingInfo anti_targeting;

  if (const absl::optional<int> version = dict.FindInt("version")) {
    if (kAntiTargetingResourceVersion.Get() != *version) {
      return base::unexpected("Failed to load from JSON, version mismatch");
    }

    anti_targeting.version = *version;
  }

  const auto* const sites_dict = dict.FindDict("sites");
  if (!sites_dict) {
    return base::unexpected("Failed to load from JSON, sites missing");
  }

  for (const auto [creative_set_id, sites] : *sites_dict) {
    if (!sites.is_list()) {
      return base::unexpected(
          "Failed to load from JSON, sites not of type list");
    }

    std::set<GURL> anti_targeting_sites;
    for (const auto& site : sites.GetList()) {
      if (!site.is_string()) {
        return base::unexpected(
            "Failed to load from JSON, site not of type string");
      }
      anti_targeting_sites.insert(GURL(site.GetString()));
    }

    anti_targeting.sites[creative_set_id] = std::move(anti_targeting_sites);
  }

  return anti_targeting;
}

}  // namespace brave_ads
