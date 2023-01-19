/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_info.h"

#include "absl/types/optional.h"
#include "base/values.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_features.h"
#include "url/gurl.h"

namespace ads::resource {

AntiTargetingInfo::AntiTargetingInfo() = default;

AntiTargetingInfo::AntiTargetingInfo(const AntiTargetingInfo& other) = default;

AntiTargetingInfo& AntiTargetingInfo::operator=(
    const AntiTargetingInfo& other) = default;

AntiTargetingInfo::AntiTargetingInfo(AntiTargetingInfo&& other) noexcept =
    default;

AntiTargetingInfo& AntiTargetingInfo::operator=(
    AntiTargetingInfo&& other) noexcept = default;

AntiTargetingInfo::~AntiTargetingInfo() = default;

// static
std::unique_ptr<AntiTargetingInfo> AntiTargetingInfo::CreateFromValue(
    base::Value resource_value,
    std::string* error_message) {
  DCHECK(error_message);
  auto anti_targeting = std::make_unique<AntiTargetingInfo>();

  if (!resource_value.is_dict()) {
    *error_message = "Failed to load from JSON, json is not a dictionary";
    return {};
  }

  const base::Value::Dict& resource = resource_value.GetDict();
  if (absl::optional<int> version = resource.FindInt("version")) {
    if (features::GetAntiTargetingResourceVersion() != *version) {
      *error_message = "Failed to load from JSON, version missing";
      return {};
    }

    anti_targeting->version = *version;
  }

  const base::Value::Dict* const site_lists = resource.FindDict("sites");
  if (!site_lists) {
    *error_message = "Failed to load from JSON, sites missing";
    return {};
  }

  for (const auto [key, value] : *site_lists) {
    if (!value.is_list()) {
      *error_message = "Failed to load from JSON, sites not of type list";
      return {};
    }

    std::set<GURL> sites;
    for (const auto& site : value.GetList()) {
      sites.insert(GURL(site.GetString()));
    }

    anti_targeting->sites.insert({key, sites});
  }

  return anti_targeting;
}

}  // namespace ads::resource
