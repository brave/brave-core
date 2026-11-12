/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_flattener.h"

#include <string>
#include <string_view>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/component_order_id_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/component_order_value_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_page_type.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_selector_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_id_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_value_extraction_patterns_info.h"

namespace brave_ads {

namespace {

// Detected language's regex first, since it is usually correct, then the
// remaining languages as a fallback for mixed-language or misdetected pages.
std::vector<std::string> OrderRegexesByLanguage(
    const base::flat_map<std::string, std::string>& regexes_by_language,
    std::string_view detected_language) {
  std::vector<std::string> ordered_regexes;
  ordered_regexes.reserve(regexes_by_language.size());

  if (const auto language_iter = regexes_by_language.find(detected_language);
      language_iter != regexes_by_language.cend()) {
    ordered_regexes.push_back(language_iter->second);
  }
  for (const auto& [language, regex] : regexes_by_language) {
    if (language != detected_language) {
      ordered_regexes.push_back(regex);
    }
  }

  return ordered_regexes;
}

// `kCheckout` is tried before `kThankYou`, since a tab may already be past
// the checkout page by the time extraction runs.
OrderExtractionSelectors FlattenSelectorsForDomain(
    const base::flat_map<
        std::string,
        base::flat_map<OrderExtractionPageType, OrderExtractionSelectors>>&
        selectors_by_domain,
    base::span<const std::string> domain_candidates) {
  for (const std::string& domain : domain_candidates) {
    const auto domain_iter = selectors_by_domain.find(domain);
    if (domain_iter == selectors_by_domain.cend()) {
      continue;
    }

    OrderExtractionSelectors flattened_order_extraction_selectors;
    for (const OrderExtractionPageType page_type :
         {OrderExtractionPageType::kCheckout,
          OrderExtractionPageType::kThankYou}) {
      const auto page_type_iter = domain_iter->second.find(page_type);
      if (page_type_iter != domain_iter->second.cend()) {
        flattened_order_extraction_selectors.insert(
            flattened_order_extraction_selectors.end(),
            page_type_iter->second.cbegin(), page_type_iter->second.cend());
      }
    }
    if (!flattened_order_extraction_selectors.empty()) {
      return flattened_order_extraction_selectors;
    }
  }

  return {};
}

}  // namespace

OrderIdExtractionPatternsInfo FlattenOrderIdExtractionPatterns(
    const ComponentOrderIdExtractionPatternsInfo& order_id_extraction_patterns,
    base::span<const std::string> domain_candidates,
    std::string_view detected_language) {
  OrderIdExtractionPatternsInfo flattened_order_id_extraction_patterns;

  flattened_order_id_extraction_patterns.selectors = FlattenSelectorsForDomain(
      order_id_extraction_patterns.selectors, domain_candidates);
  flattened_order_id_extraction_patterns.regexes = OrderRegexesByLanguage(
      order_id_extraction_patterns.regexes, detected_language);

  return flattened_order_id_extraction_patterns;
}

OrderValueExtractionPatternsInfo FlattenOrderValueExtractionPatterns(
    const ComponentOrderValueExtractionPatternsInfo&
        order_value_extraction_patterns,
    base::span<const std::string> domain_candidates,
    std::string_view detected_language) {
  OrderValueExtractionPatternsInfo flattened_order_value_extraction_patterns;

  flattened_order_value_extraction_patterns.selectors =
      FlattenSelectorsForDomain(order_value_extraction_patterns.selectors,
                                domain_candidates);
  // Keeps the currency code as the map key, so a matching currency's
  // identity survives into `OrderInfo` rather than being discarded here.
  flattened_order_value_extraction_patterns.currency_regexes =
      order_value_extraction_patterns.currency_regexes;
  flattened_order_value_extraction_patterns.keyword_regexes =
      OrderRegexesByLanguage(order_value_extraction_patterns.keyword_regexes,
                             detected_language);
  flattened_order_value_extraction_patterns
      .number_of_ancestor_levels_to_search =
      order_value_extraction_patterns.number_of_ancestor_levels_to_search;

  return flattened_order_value_extraction_patterns;
}

}  // namespace brave_ads
