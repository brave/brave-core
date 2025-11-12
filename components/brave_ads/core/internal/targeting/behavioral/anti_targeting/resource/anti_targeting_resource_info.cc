/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource_info.h"

#include <optional>
#include <string_view>

#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/anti_targeting_feature.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

constexpr std::string_view kVersionKey = "version";
constexpr std::string_view kSitesKey = "sites";

}  // namespace

AntiTargetingResourceInfo::AntiTargetingResourceInfo() = default;

AntiTargetingResourceInfo::AntiTargetingResourceInfo(
    AntiTargetingResourceInfo&& other) noexcept = default;

AntiTargetingResourceInfo& AntiTargetingResourceInfo::operator=(
    AntiTargetingResourceInfo&& other) noexcept = default;

AntiTargetingResourceInfo::~AntiTargetingResourceInfo() = default;

// static
std::optional<AntiTargetingResourceInfo>
AntiTargetingResourceInfo::CreateFromValue(const base::Value::Dict dict) {
  AntiTargetingResourceInfo anti_targeting;

  if (std::optional<int> version = dict.FindInt(kVersionKey)) {
    if (version != kAntiTargetingResourceVersion.Get()) {
      return std::nullopt;
    }

    anti_targeting.version = version;
  }

  const auto* const sites_dict = dict.FindDict(kSitesKey);
  if (!sites_dict) {
    return std::nullopt;
  }

  for (const auto [creative_set_id, sites] : *sites_dict) {
    if (!sites.is_list()) {
      return std::nullopt;
    }

    for (const auto& site : sites.GetList()) {
      if (!site.is_string()) {
        return std::nullopt;
      }

      anti_targeting.creative_sets[creative_set_id].insert(
          GURL(site.GetString()));
    }
  }

  return anti_targeting;
}

}  // namespace brave_ads
