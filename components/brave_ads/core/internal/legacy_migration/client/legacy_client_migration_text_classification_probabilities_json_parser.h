/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_CLIENT_LEGACY_CLIENT_MIGRATION_TEXT_CLASSIFICATION_PROBABILITIES_JSON_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_CLIENT_LEGACY_CLIENT_MIGRATION_TEXT_CLASSIFICATION_PROBABILITIES_JSON_PARSER_H_

#include <optional>
#include <string_view>

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_types.h"

namespace brave_ads::json::reader {

// Parses the history of text classification probabilities from the legacy
// `client.json` `textClassificationProbabilitiesHistory` list, ordered
// newest first as it was stored. Returns `std::nullopt` if `json` is
// malformed.
std::optional<TextClassificationProbabilityList>
ParseTextClassificationProbabilities(std::string_view json);

}  // namespace brave_ads::json::reader

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_CLIENT_LEGACY_CLIENT_MIGRATION_TEXT_CLASSIFICATION_PROBABILITIES_JSON_PARSER_H_
