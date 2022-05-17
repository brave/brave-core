/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_info.h"

#include <set>

#include "base/values.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_features.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace resource {

AntiTargetingInfo::AntiTargetingInfo() = default;

AntiTargetingInfo::AntiTargetingInfo(const AntiTargetingInfo& info) = default;

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

  if (absl::optional<int> version = resource_value.FindIntPath("version")) {
    if (features::GetAntiTargetingResourceVersion() != *version) {
      *error_message = "Failed to load from JSON, version missing";
      return {};
    }

    anti_targeting->version = *version;
  }

  base::Value* site_lists_value = resource_value.FindDictPath("sites");
  if (!site_lists_value) {
    *error_message = "Failed to load from JSON, sites missing";
    return {};
  }

  if (!site_lists_value->is_dict()) {
    *error_message = "Failed to load from JSON, sites not of type dict";
    return {};
  }

  base::DictionaryValue* dict;
  if (!site_lists_value->GetAsDictionary(&dict)) {
    *error_message = "Failed to load from JSON, get sites as dict";
    return {};
  }

  for (base::DictionaryValue::Iterator iter(*dict); !iter.IsAtEnd();
       iter.Advance()) {
    if (!iter.value().is_list()) {
      *error_message = "Failed to load from JSON, sites not of type list";
      return {};
    }

    std::set<GURL> sites;
    for (const auto& site : iter.value().GetList()) {
      sites.insert(GURL(site.GetString()));
    }

    anti_targeting->sites.insert({iter.key(), sites});
  }

  return anti_targeting;
}

}  // namespace resource
}  // namespace ads
