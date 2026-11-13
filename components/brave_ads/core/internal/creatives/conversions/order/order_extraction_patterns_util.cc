/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_util.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/component_order_id_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/component_order_value_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_page_type.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_selector_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_id_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_value_extraction_patterns_info.h"

namespace brave_ads {

namespace {

// Returns the regexes for the detected language first, followed by the
// remaining languages.
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

// Returns the order extraction selectors for the most specific site first,
// based on the given site candidates.
OrderExtractionSelectors ResolveSelectorsForSite(
    const base::flat_map<
        std::string,
        base::flat_map<OrderExtractionPageType, OrderExtractionSelectors>>&
        selectors_by_site,
    base::span<const std::string> site_candidates) {
  for (const std::string& site : site_candidates) {
    const auto site_iter = selectors_by_site.find(site);
    if (site_iter == selectors_by_site.cend()) {
      continue;
    }

    OrderExtractionSelectors order_extraction_selectors;
    for (const OrderExtractionPageType page_type :
         {OrderExtractionPageType::kCheckout,
          OrderExtractionPageType::kThankYou}) {
      const auto page_type_iter = site_iter->second.find(page_type);
      if (page_type_iter != site_iter->second.cend()) {
        order_extraction_selectors.insert(order_extraction_selectors.end(),
                                          page_type_iter->second.cbegin(),
                                          page_type_iter->second.cend());
      }
    }
    if (!order_extraction_selectors.empty()) {
      return order_extraction_selectors;
    }
  }

  return {};
}

// Returns the validation pattern for the most specific site first, based on the
// given site candidates, or the default pattern if none are configured.
std::optional<std::string> ResolveValidationPatternForSite(
    const base::flat_map<std::string, std::string>& pattern_overrides,
    base::span<const std::string> site_candidates,
    const std::string& default_pattern) {
  for (const std::string& site : site_candidates) {
    if (const auto override_iter = pattern_overrides.find(site);
        override_iter != pattern_overrides.cend()) {
      return override_iter->second;
    }
  }

  if (!default_pattern.empty()) {
    return default_pattern;
  }

  return std::nullopt;
}

// Returns the search bound for the most specific site first, based on the given
// site candidates, or the default bound if none are configured.
int ResolveSearchBoundForSite(
    const base::flat_map<std::string, int>& bound_overrides,
    base::span<const std::string> site_candidates,
    int default_bound) {
  for (const std::string& site : site_candidates) {
    if (const auto override_iter = bound_overrides.find(site);
        override_iter != bound_overrides.cend()) {
      return override_iter->second;
    }
  }

  return default_bound;
}

}  // namespace

OrderIdExtractionPatternsInfo ToOrderIdExtractionPatterns(
    const ComponentOrderIdExtractionPatternsInfo&
        component_order_id_extraction_patterns,
    base::span<const std::string> site_candidates,
    std::string_view detected_language) {
  OrderIdExtractionPatternsInfo order_id_extraction_patterns;

  order_id_extraction_patterns.selectors = ResolveSelectorsForSite(
      component_order_id_extraction_patterns.selectors, site_candidates);
  order_id_extraction_patterns.regexes = OrderRegexesByLanguage(
      component_order_id_extraction_patterns.regexes, detected_language);
  order_id_extraction_patterns.max_combined_text_length_to_search =
      ResolveSearchBoundForSite(
          component_order_id_extraction_patterns
              .max_combined_text_length_to_search_overrides,
          site_candidates,
          component_order_id_extraction_patterns
              .default_max_combined_text_length_to_search);
  order_id_extraction_patterns.number_of_descendant_levels_to_search =
      ResolveSearchBoundForSite(
          component_order_id_extraction_patterns
              .number_of_descendant_levels_to_search_overrides,
          site_candidates,
          component_order_id_extraction_patterns
              .default_number_of_descendant_levels_to_search);
  order_id_extraction_patterns.validation_pattern =
      ResolveValidationPatternForSite(
          component_order_id_extraction_patterns.validation_pattern_overrides,
          site_candidates,
          component_order_id_extraction_patterns.default_validation_pattern);

  return order_id_extraction_patterns;
}

OrderValueExtractionPatternsInfo ToOrderValueExtractionPatterns(
    const ComponentOrderValueExtractionPatternsInfo&
        component_order_value_extraction_patterns,
    base::span<const std::string> site_candidates,
    std::string_view detected_language) {
  OrderValueExtractionPatternsInfo order_value_extraction_patterns;

  order_value_extraction_patterns.selectors = ResolveSelectorsForSite(
      component_order_value_extraction_patterns.selectors, site_candidates);
  // Keeps the currency code as the map key, so a matching currency's
  // identity survives into `OrderInfo` rather than being discarded here.
  order_value_extraction_patterns.currency_regexes =
      component_order_value_extraction_patterns.currency_regexes;
  order_value_extraction_patterns.keyword_regexes = OrderRegexesByLanguage(
      component_order_value_extraction_patterns.keyword_regexes,
      detected_language);
  order_value_extraction_patterns.number_of_ancestor_levels_to_search =
      ResolveSearchBoundForSite(
          component_order_value_extraction_patterns
              .number_of_ancestor_levels_to_search_overrides,
          site_candidates,
          component_order_value_extraction_patterns
              .default_number_of_ancestor_levels_to_search);

  return order_value_extraction_patterns;
}

}  // namespace brave_ads
