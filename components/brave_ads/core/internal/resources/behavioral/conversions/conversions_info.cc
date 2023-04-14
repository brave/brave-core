/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/behavioral/conversions/conversions_info.h"

#include <utility>

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions_features.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads::resource {

ConversionsInfo::ConversionsInfo() = default;
ConversionsInfo::ConversionsInfo(ConversionsInfo&& other) noexcept = default;
ConversionsInfo& ConversionsInfo::operator=(ConversionsInfo&& other) noexcept =
    default;
ConversionsInfo::~ConversionsInfo() = default;

// static
base::expected<ConversionsInfo, std::string> ConversionsInfo::CreateFromValue(
    const base::Value::Dict dict) {
  ConversionsInfo conversion;

  if (absl::optional<int> version = dict.FindInt("version")) {
    if (kConversionsResourceVersion.Get() != *version) {
      return base::unexpected("Failed to load from JSON, version mismatch");
    }
    conversion.version = *version;
  }

  const base::Value::Dict* conversion_id_patterns_value =
      dict.FindDict("conversion_id_patterns");
  if (!conversion_id_patterns_value) {
    return base::unexpected(
        "Failed to load from JSON, conversion patterns missing");
  }

  for (const auto [key, value] : *conversion_id_patterns_value) {
    if (!value.is_dict()) {
      return base::unexpected(
          "Failed to load from JSON, conversion pattern not of type dict");
    }

    const std::string* const id_pattern = value.FindStringKey("id_pattern");
    if (!id_pattern || id_pattern->empty()) {
      return base::unexpected(
          "Failed to load from JSON, pattern id_pattern missing");
    }

    const std::string* const search_in = value.FindStringKey("search_in");
    if (!search_in || search_in->empty()) {
      return base::unexpected(
          "Failed to load from JSON, pattern search_in missing");
    }

    ConversionIdPatternInfo conversion_id_pattern;
    conversion_id_pattern.id_pattern = *id_pattern;
    conversion_id_pattern.search_in = *search_in;
    conversion_id_pattern.url_pattern = key;

    conversion.id_patterns[key] = std::move(conversion_id_pattern);
  }

  return conversion;
}

}  // namespace brave_ads::resource
