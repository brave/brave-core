/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/conversions/conversions_info.h"

#include "absl/types/optional.h"
#include "base/values.h"
#include "bat/ads/internal/conversions/conversions_features.h"

namespace ads::resource {

ConversionsInfo::ConversionsInfo() = default;

ConversionsInfo::ConversionsInfo(const ConversionsInfo& other) = default;

ConversionsInfo& ConversionsInfo::operator=(const ConversionsInfo& other) =
    default;

ConversionsInfo::ConversionsInfo(ConversionsInfo&& other) noexcept = default;

ConversionsInfo& ConversionsInfo::operator=(ConversionsInfo&& other) noexcept =
    default;

ConversionsInfo::~ConversionsInfo() = default;

// static
std::unique_ptr<ConversionsInfo> ConversionsInfo::CreateFromValue(
    base::Value resource_value,
    std::string* error_message) {
  DCHECK(error_message);
  auto conversion = std::make_unique<ConversionsInfo>();

  if (!resource_value.is_dict()) {
    *error_message = "Failed to load from JSON, json is not a dictionary";
    return {};
  }

  if (absl::optional<int> version = resource_value.FindIntKey("version")) {
    if (features::GetConversionsResourceVersion() != *version) {
      *error_message = "Failed to load from JSON, version missing";
      return {};
    }
    conversion->version = *version;
  }

  base::Value* conversion_id_patterns_value =
      resource_value.FindDictKey("conversion_id_patterns");
  if (!conversion_id_patterns_value) {
    *error_message = "Failed to load from JSON, conversion patterns missing";
    return {};
  }

  for (const auto item : conversion_id_patterns_value->DictItems()) {
    if (!item.second.is_dict()) {
      *error_message =
          "Failed to load from JSON, conversion pattern not of type dict";
      return {};
    }

    const std::string* const id_pattern =
        item.second.FindStringKey("id_pattern");
    if (!id_pattern || id_pattern->empty()) {
      *error_message = "Failed to load from JSON, pattern id_pattern missing";
      return {};
    }

    const std::string* const search_in = item.second.FindStringKey("search_in");
    if (!search_in || search_in->empty()) {
      *error_message = "Failed to load from JSON, pattern search_in missing";
      return {};
    }

    ConversionIdPatternInfo info;
    info.id_pattern = *id_pattern;
    info.search_in = *search_in;
    info.url_pattern = item.first;
    conversion->id_patterns.insert({info.url_pattern, info});
  }

  return conversion;
}

}  // namespace ads::resource
