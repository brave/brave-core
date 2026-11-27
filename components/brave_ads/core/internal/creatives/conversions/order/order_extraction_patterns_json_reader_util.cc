/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_json_reader_util.h"

#include <utility>

#include "base/values.h"
#include "third_party/re2/src/re2/re2.h"

namespace brave_ads::json::reader {

namespace {

constexpr std::string_view kCheckoutPageTypeValue = "checkout";
constexpr std::string_view kThankYouPageTypeValue = "thankYou";
constexpr std::string_view kSelectorKey = "selector";
constexpr std::string_view kAttributeNameKey = "attributeName";
constexpr std::string_view kPatternKey = "pattern";
constexpr std::string_view kValidationKey = "validation";

bool IsCompilableRegex(const std::string& pattern) {
  const re2::RE2 regex(pattern, re2::RE2::Quiet);
  return regex.ok();
}

}  // namespace

std::optional<OrderExtractionPageType> MaybeParsePageType(
    std::string_view page_type) {
  if (page_type == kCheckoutPageTypeValue) {
    return OrderExtractionPageType::kCheckout;
  }
  if (page_type == kThankYouPageTypeValue) {
    return OrderExtractionPageType::kThankYou;
  }
  return std::nullopt;
}

std::optional<OrderExtractionSelectorInfo> MaybeParseSelector(
    const base::DictValue& dict) {
  const std::string* const selector = dict.FindString(kSelectorKey);
  if (!selector || selector->empty()) {
    return std::nullopt;
  }

  OrderExtractionSelectorInfo order_extraction_selector;
  order_extraction_selector.selector = *selector;
  if (const std::string* const attribute_name =
          dict.FindString(kAttributeNameKey)) {
    // An empty attribute name can never match any element's attribute, so
    // it is rejected outright rather than silently never matching.
    if (attribute_name->empty()) {
      return std::nullopt;
    }
    order_extraction_selector.attribute_name = *attribute_name;
  }
  if (const std::string* const pattern = dict.FindString(kPatternKey)) {
    order_extraction_selector.pattern = *pattern;
  }
  if (const base::DictValue* const validation_dict =
          dict.FindDict(kValidationKey)) {
    if (const std::string* const validation_pattern =
            validation_dict->FindString(kPatternKey)) {
      // Unlike a malformed extraction pattern, a present validation `pattern`
      // that is empty or fails to compile rejects the whole update.
      if (validation_pattern->empty() ||
          !IsCompilableRegex(*validation_pattern)) {
        return std::nullopt;
      }
      order_extraction_selector.validation_pattern = *validation_pattern;
    }
  }
  return order_extraction_selector;
}

std::optional<OrderExtractionSelectors> MaybeParseSelectorList(
    const base::ListValue& list) {
  OrderExtractionSelectors order_extraction_selectors;
  for (const auto& value : list) {
    const base::DictValue* const selector_dict = value.GetIfDict();
    if (!selector_dict) {
      return std::nullopt;
    }
    std::optional<OrderExtractionSelectorInfo> order_extraction_selector =
        MaybeParseSelector(*selector_dict);
    if (!order_extraction_selector) {
      return std::nullopt;
    }
    order_extraction_selectors.push_back(std::move(*order_extraction_selector));
  }
  return order_extraction_selectors;
}

std::optional<base::flat_map<OrderExtractionPageType, OrderExtractionSelectors>>
MaybeParseSelectorsByPageType(const base::DictValue& dict) {
  base::flat_map<OrderExtractionPageType, OrderExtractionSelectors>
      selectors_by_page_type;

  for (const auto [page_type, selectors] : dict) {
    std::optional<OrderExtractionPageType> parsed_page_type =
        MaybeParsePageType(page_type);
    if (!parsed_page_type) {
      // Not a recognized key (e.g. `validation`, `search`); tolerated for
      // forward compatibility, not a rejection.
      continue;
    }

    const base::ListValue* const list = selectors.GetIfList();
    if (!list) {
      return std::nullopt;
    }
    std::optional<OrderExtractionSelectors> order_extraction_selectors =
        MaybeParseSelectorList(*list);
    if (!order_extraction_selectors) {
      return std::nullopt;
    }
    if (!order_extraction_selectors->empty()) {
      selectors_by_page_type[*parsed_page_type] =
          std::move(*order_extraction_selectors);
    }
  }
  return selectors_by_page_type;
}

std::optional<int> MaybeParseNonNegativeInt(const base::Value& value) {
  std::optional<int> int_value = value.GetIfInt();
  if (int_value < 0) {
    return std::nullopt;
  }
  return int_value;
}

std::optional<std::string> MaybeParseValidationPattern(
    const base::Value& value) {
  const std::string* const pattern = value.GetIfString();
  if (!pattern || pattern->empty() || !IsCompilableRegex(*pattern)) {
    return std::nullopt;
  }
  return *pattern;
}

std::optional<base::flat_map<std::string, std::string>>
MaybeParseCompiledRegexes(const base::Value& value) {
  const base::DictValue* const regex_dict = value.GetIfDict();
  if (!regex_dict || regex_dict->empty()) {
    return std::nullopt;
  }

  base::flat_map<std::string, std::string> regexes;
  for (const auto [language_code, entry_value] : *regex_dict) {
    const std::string* const pattern = entry_value.GetIfString();
    if (!pattern || pattern->empty()) {
      // Unlike a pattern that fails to compile below, a non-string or empty
      // entry is malformed content, so it rejects the whole map.
      return std::nullopt;
    }
    if (IsCompilableRegex(*pattern)) {
      regexes[language_code] = *pattern;
    }
  }
  if (regexes.empty()) {
    // Every entry failed to compile, so this is treated as malformed rather
    // than an empty-but-valid map.
    return std::nullopt;
  }
  return regexes;
}

}  // namespace brave_ads::json::reader
