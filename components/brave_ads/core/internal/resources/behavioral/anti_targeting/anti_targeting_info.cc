/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_info.h"

#include <utility>

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_features.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace brave_ads::resource {

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

  if (absl::optional<int> version = dict.FindInt("version")) {
    if (kAntiTargetingResourceVersion.Get() != *version) {
      return base::unexpected("Failed to load from JSON, version mismatch");
    }

    anti_targeting.version = *version;
  }

  const base::Value::Dict* const site_lists = dict.FindDict("sites");
  if (!site_lists) {
    return base::unexpected("Failed to load from JSON, sites missing");
  }

  for (const auto [key, value] : *site_lists) {
    if (!value.is_list()) {
      return base::unexpected(
          "Failed to load from JSON, sites not of type list");
    }

    std::set<GURL> sites;
    for (const auto& site : value.GetList()) {
      sites.insert(GURL(site.GetString()));
    }

    anti_targeting.sites[key] = std::move(sites);
  }

  return anti_targeting;
}

}  // namespace brave_ads::resource
