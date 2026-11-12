/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_json_reader.h"

#include <string>
#include <string_view>

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/component_order_id_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/component_order_value_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_json_reader_util.h"

namespace brave_ads::json::reader {

namespace {

constexpr std::string_view kRegexKey = "regex";
constexpr std::string_view kSelectorsKey = "selectors";
constexpr std::string_view kCurrencyKey = "currency";
constexpr std::string_view kKeywordKey = "keyword";
constexpr std::string_view kNumberOfAncestorLevelsToSearchKey =
    "numberOfAncestorLevelsToSearch";
constexpr std::string_view kIdKey = "id";
constexpr std::string_view kValueKey = "value";
constexpr std::string_view kSourceKey = "source";
constexpr std::string_view kPatternsKey = "patterns";

ComponentOrderIdExtractionPatternsInfo ParseIdPatterns(
    const base::DictValue* const dict) {
  ComponentOrderIdExtractionPatternsInfo id_patterns;
  if (!dict) {
    return id_patterns;
  }

  if (const base::DictValue* const regex_dict = dict->FindDict(kRegexKey)) {
    if (const std::string* const source = regex_dict->FindString(kSourceKey)) {
      id_patterns.source = *source;
    }
    id_patterns.regexes = ParseStringMap(regex_dict->FindDict(kPatternsKey));
  }
  id_patterns.selectors = ParseSelectorsByDomain(dict->FindDict(kSelectorsKey));
  return id_patterns;
}

ComponentOrderValueExtractionPatternsInfo ParseValuePatterns(
    const base::DictValue* const dict) {
  ComponentOrderValueExtractionPatternsInfo value_patterns;
  if (!dict) {
    return value_patterns;
  }

  if (const base::DictValue* const regex_dict = dict->FindDict(kRegexKey)) {
    value_patterns.currency_regexes =
        ParseStringMap(regex_dict->FindDict(kCurrencyKey));
    value_patterns.keyword_regexes =
        ParseStringMap(regex_dict->FindDict(kKeywordKey));
    value_patterns.number_of_ancestor_levels_to_search =
        regex_dict->FindInt(kNumberOfAncestorLevelsToSearchKey).value_or(0);
  }
  value_patterns.selectors =
      ParseSelectorsByDomain(dict->FindDict(kSelectorsKey));
  return value_patterns;
}

}  // namespace

OrderExtractionPatternsInfo ReadOrderPatterns(
    const base::DictValue& order_dict) {
  OrderExtractionPatternsInfo patterns;
  patterns.id = ParseIdPatterns(order_dict.FindDict(kIdKey));
  patterns.value = ParseValuePatterns(order_dict.FindDict(kValueKey));
  return patterns;
}

}  // namespace brave_ads::json::reader
