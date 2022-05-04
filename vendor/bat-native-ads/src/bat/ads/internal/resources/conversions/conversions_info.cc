/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/conversions/conversions_info.h"

#include "base/values.h"
#include "bat/ads/internal/conversions/conversions_features.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace resource {

ConversionsInfo::ConversionsInfo() = default;

ConversionsInfo::~ConversionsInfo() = default;

// static
std::unique_ptr<ConversionsInfo> ConversionsInfo::CreateFromValue(
    base::Value resource_value,
    std::string* error_message) {
  DCHECK(error_message);
  auto conversions_info = std::make_unique<ConversionsInfo>();

  if (!resource_value.is_dict()) {
    *error_message = "Failed to load from JSON, json is not a dictionary";
    return {};
  }

  if (absl::optional<int> version = resource_value.FindIntKey("version")) {
    if (features::GetConversionsResourceVersion() != *version) {
      *error_message = "Failed to load from JSON, version missing";
      return {};
    }
    conversions_info->version = *version;
  }

  base::Value* conversion_id_patterns_value =
      resource_value.FindDictKey("conversion_id_patterns");
  if (!conversion_id_patterns_value) {
    *error_message = "Failed to load from JSON, conversion patterns missing";
    return {};
  }

  for (const auto value : conversion_id_patterns_value->DictItems()) {
    if (!value.second.is_dict()) {
      *error_message =
          "Failed to load from JSON, conversion pattern not of type dict";
      return {};
    }

    const std::string* id_pattern = value.second.FindStringKey("id_pattern");
    if (!id_pattern || id_pattern->empty()) {
      *error_message = "Failed to load from JSON, pattern id_pattern missing";
      return {};
    }

    const std::string* search_in = value.second.FindStringKey("search_in");
    if (!search_in || search_in->empty()) {
      *error_message = "Failed to load from JSON, pattern search_in missing";
      return {};
    }

    ConversionIdPatternInfo info;
    info.id_pattern = *id_pattern;
    info.search_in = *search_in;
    info.url_pattern = value.first;
    conversions_info->conversion_id_patterns.insert({info.url_pattern, info});
  }

  return conversions_info;
}

}  // namespace resource
}  // namespace ads
