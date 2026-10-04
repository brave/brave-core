/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration_text_classification_probabilities_json_parser.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"

namespace brave_ads::json::reader {

namespace {

constexpr char kProbabilitiesHistoryKey[] =
    "textClassificationProbabilitiesHistory";
constexpr char kProbabilitiesKey[] = "textClassificationProbabilities";
constexpr char kSegmentKey[] = "segment";
constexpr char kPageScoreKey[] = "pageScore";

}  // namespace

std::optional<TextClassificationProbabilityList>
ParseTextClassificationProbabilities(std::string_view json) {
  std::optional<base::DictValue> dict =
      base::JSONReader::ReadDict(json, base::JSON_PARSE_RFC);
  if (!dict) {
    return std::nullopt;
  }

  TextClassificationProbabilityList text_classification_probabilities;

  const auto* const history = dict->FindList(kProbabilitiesHistoryKey);
  if (!history) {
    // No text classification probabilities to migrate.
    return text_classification_probabilities;
  }

  for (const auto& entry : *history) {
    const auto* const entry_dict = entry.GetIfDict();
    if (!entry_dict) {
      BLOG(0,
           "Text classification probability history entry should be a "
           "dictionary");
      continue;
    }

    const auto* const probabilities_list =
        entry_dict->FindList(kProbabilitiesKey);
    if (!probabilities_list) {
      continue;
    }

    TextClassificationProbabilityMap probabilities;

    for (const auto& item : *probabilities_list) {
      const auto* const item_dict = item.GetIfDict();
      if (!item_dict) {
        BLOG(0, "Text classification probability should be a dictionary");
        continue;
      }

      const std::string* const segment = item_dict->FindString(kSegmentKey);
      if (!segment) {
        continue;
      }

      double page_score = 0.0;
      if (const auto page_score_value = item_dict->FindDouble(kPageScoreKey)) {
        page_score = *page_score_value;
      } else if (const auto* const legacy_page_score_value =
                     item_dict->FindString(kPageScoreKey)) {
        if (!base::StringToDouble(*legacy_page_score_value, &page_score)) {
          BLOG(0, "Failed to parse legacy page score from client state");
          continue;
        }
      }

      probabilities.insert({*segment, page_score});
    }

    if (!probabilities.empty()) {
      text_classification_probabilities.push_back(std::move(probabilities));
    }
  }

  return text_classification_probabilities;
}

}  // namespace brave_ads::json::reader
