/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_util.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/extend.h"
#include "base/containers/flat_map.h"
#include "base/containers/map_util.h"
#include "base/containers/span.h"
#include "base/numerics/safe_conversions.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/component_order_id_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/component_order_value_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_page_type.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_selector_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

namespace {

std::vector<mojom::OrderExtractionSelectorInfoPtr> ToMojomSelectors(
    OrderExtractionSelectors selectors) {
  std::vector<mojom::OrderExtractionSelectorInfoPtr> mojom_selectors;
  mojom_selectors.reserve(selectors.size());

  for (auto& selector : selectors) {
    mojom::OrderExtractionSelectorInfoPtr mojom_selector =
        mojom::OrderExtractionSelectorInfo::New();
    mojom_selector->selector = std::move(selector.selector);
    mojom_selector->attribute_name = std::move(selector.attribute_name);
    mojom_selector->pattern = std::move(selector.pattern);
    mojom_selector->validation_pattern = std::move(selector.validation_pattern);
    mojom_selectors.push_back(std::move(mojom_selector));
  }

  return mojom_selectors;
}

// Returns the regexes for the detected language first, followed by the
// remaining languages. With no `detected_language`, no lookup is attempted
// and every language is returned in schema order.
std::vector<std::string> OrderRegexesByLanguage(
    const base::flat_map<std::string, std::string>& regexes_by_language,
    std::optional<std::string_view> detected_language) {
  std::vector<std::string> ordered_regexes;
  ordered_regexes.reserve(regexes_by_language.size());

  if (detected_language) {
    if (const auto language_iter = regexes_by_language.find(*detected_language);
        language_iter != regexes_by_language.cend()) {
      ordered_regexes.push_back(language_iter->second);
    }
  }
  for (const auto& [language, regex] : regexes_by_language) {
    if (!detected_language || language != *detected_language) {
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
    const auto* const selectors_by_page_type =
        base::FindOrNull(selectors_by_site, site);
    if (!selectors_by_page_type) {
      continue;
    }

    OrderExtractionSelectors order_extraction_selectors;
    for (const OrderExtractionPageType page_type :
         {OrderExtractionPageType::kCheckout,
          OrderExtractionPageType::kThankYou}) {
      if (const auto* const selectors =
              base::FindOrNull(*selectors_by_page_type, page_type)) {
        base::Extend(order_extraction_selectors, base::span(*selectors));
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
    if (const auto* const override_pattern =
            base::FindOrNull(pattern_overrides, site)) {
      return *override_pattern;
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
    if (const auto* const override_bound =
            base::FindOrNull(bound_overrides, site)) {
      return *override_bound;
    }
  }

  return default_bound;
}

}  // namespace

mojom::OrderIdExtractionPatternsInfoPtr ToOrderIdExtractionPatterns(
    const ComponentOrderIdExtractionPatternsInfo&
        component_order_id_extraction_patterns,
    base::span<const std::string> site_candidates,
    std::optional<std::string_view> detected_language) {
  mojom::OrderIdExtractionPatternsInfoPtr order_id_extraction_patterns =
      mojom::OrderIdExtractionPatternsInfo::New();

  order_id_extraction_patterns->selectors =
      ToMojomSelectors(ResolveSelectorsForSite(
          component_order_id_extraction_patterns.selectors, site_candidates));
  order_id_extraction_patterns->regexes = OrderRegexesByLanguage(
      component_order_id_extraction_patterns.regexes, detected_language);
  order_id_extraction_patterns->max_combined_text_length_to_search =
      base::saturated_cast<uint32_t>(ResolveSearchBoundForSite(
          component_order_id_extraction_patterns
              .max_combined_text_length_to_search_overrides,
          site_candidates,
          component_order_id_extraction_patterns
              .default_max_combined_text_length_to_search));
  order_id_extraction_patterns->number_of_descendant_levels_to_search =
      base::saturated_cast<uint32_t>(ResolveSearchBoundForSite(
          component_order_id_extraction_patterns
              .number_of_descendant_levels_to_search_overrides,
          site_candidates,
          component_order_id_extraction_patterns
              .default_number_of_descendant_levels_to_search));
  order_id_extraction_patterns->validation_pattern =
      ResolveValidationPatternForSite(
          component_order_id_extraction_patterns.validation_pattern_overrides,
          site_candidates,
          component_order_id_extraction_patterns.default_validation_pattern);

  return order_id_extraction_patterns;
}

mojom::OrderValueExtractionPatternsInfoPtr ToOrderValueExtractionPatterns(
    const ComponentOrderValueExtractionPatternsInfo&
        component_order_value_extraction_patterns,
    base::span<const std::string> site_candidates,
    std::optional<std::string_view> detected_language) {
  mojom::OrderValueExtractionPatternsInfoPtr order_value_extraction_patterns =
      mojom::OrderValueExtractionPatternsInfo::New();

  order_value_extraction_patterns->selectors =
      ToMojomSelectors(ResolveSelectorsForSite(
          component_order_value_extraction_patterns.selectors,
          site_candidates));
  // Keeps the currency code as the map key, so a matching currency's
  // identity survives into `OrderInfo` rather than being discarded here.
  order_value_extraction_patterns->currency_regexes =
      component_order_value_extraction_patterns.currency_regexes;
  order_value_extraction_patterns->keyword_regexes = OrderRegexesByLanguage(
      component_order_value_extraction_patterns.keyword_regexes,
      detected_language);
  order_value_extraction_patterns->number_of_ancestor_levels_to_search =
      base::saturated_cast<uint32_t>(ResolveSearchBoundForSite(
          component_order_value_extraction_patterns
              .number_of_ancestor_levels_to_search_overrides,
          site_candidates,
          component_order_value_extraction_patterns
              .default_number_of_ancestor_levels_to_search));

  return order_value_extraction_patterns;
}

}  // namespace brave_ads
