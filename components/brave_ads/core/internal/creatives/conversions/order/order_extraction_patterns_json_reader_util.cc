/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_json_reader_util.h"

#include <utility>

#include "base/values.h"

namespace brave_ads::json::reader {

namespace {

constexpr std::string_view kCheckoutPageTypeValue = "checkout";
constexpr std::string_view kThankYouPageTypeValue = "thankYou";
constexpr std::string_view kSelectorKey = "selector";
constexpr std::string_view kSourceKey = "source";
constexpr std::string_view kAttributeKey = "attribute";
constexpr std::string_view kAttributeSourceValue = "attribute";
constexpr std::string_view kPatternKey = "pattern";

}  // namespace

std::optional<OrderExtractionPageType> ParsePageType(
    std::string_view page_type) {
  if (page_type == kCheckoutPageTypeValue) {
    return OrderExtractionPageType::kCheckout;
  }
  if (page_type == kThankYouPageTypeValue) {
    return OrderExtractionPageType::kThankYou;
  }
  return std::nullopt;
}

base::flat_map<std::string, std::string> ParseStringMap(
    const base::DictValue* const dict) {
  base::flat_map<std::string, std::string> string_map;
  if (!dict) {
    return string_map;
  }
  for (const auto [key, value] : *dict) {
    if (const std::string* const regex = value.GetIfString()) {
      string_map[key] = *regex;
    }
  }
  return string_map;
}

std::optional<OrderExtractionSelectorInfo> ParseSelector(
    const base::DictValue& dict) {
  const std::string* const selector = dict.FindString(kSelectorKey);
  if (!selector) {
    return std::nullopt;
  }

  OrderExtractionSelectorInfo order_extraction_selector;
  order_extraction_selector.selector = *selector;
  const std::string* const source = dict.FindString(kSourceKey);
  if (source && *source == kAttributeSourceValue) {
    if (const std::string* const attribute_name =
            dict.FindString(kAttributeKey)) {
      order_extraction_selector.attribute_name = *attribute_name;
    }
  }
  if (const std::string* const pattern = dict.FindString(kPatternKey)) {
    order_extraction_selector.pattern = *pattern;
  }
  return order_extraction_selector;
}

OrderExtractionSelectors ParseSelectorList(const base::ListValue& list) {
  OrderExtractionSelectors order_extraction_selectors;
  for (const auto& value : list) {
    const base::DictValue* const selector_dict = value.GetIfDict();
    if (!selector_dict) {
      continue;
    }
    if (std::optional<OrderExtractionSelectorInfo> order_extraction_selector =
            ParseSelector(*selector_dict)) {
      order_extraction_selectors.push_back(
          std::move(*order_extraction_selector));
    }
  }
  return order_extraction_selectors;
}

base::flat_map<
    std::string,
    base::flat_map<OrderExtractionPageType, OrderExtractionSelectors>>
ParseSelectorsByDomain(const base::DictValue* const dict) {
  base::flat_map<std::string, base::flat_map<OrderExtractionPageType,
                                             OrderExtractionSelectors>>
      selectors_by_domain;
  if (!dict) {
    return selectors_by_domain;
  }

  for (const auto [domain, page_types] : *dict) {
    const base::DictValue* const page_type_dict = page_types.GetIfDict();
    if (!page_type_dict) {
      continue;
    }

    base::flat_map<OrderExtractionPageType, OrderExtractionSelectors>
        selectors_by_page_type;
    for (const auto [page_type, selectors] : *page_type_dict) {
      const std::optional<OrderExtractionPageType> parsed_page_type =
          ParsePageType(page_type);
      const base::ListValue* const list = selectors.GetIfList();
      if (!parsed_page_type || !list) {
        continue;
      }
      OrderExtractionSelectors order_extraction_selectors =
          ParseSelectorList(*list);
      if (!order_extraction_selectors.empty()) {
        selectors_by_page_type[*parsed_page_type] =
            std::move(order_extraction_selectors);
      }
    }
    if (!selectors_by_page_type.empty()) {
      selectors_by_domain[domain] = std::move(selectors_by_page_type);
    }
  }
  return selectors_by_domain;
}

}  // namespace brave_ads::json::reader
