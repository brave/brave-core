/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/order_patterns_mojom_util.h"

#include <utility>
#include <vector>

#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

namespace {

std::vector<mojom::OrderSelectorInfoPtr> ToMojomOrderSelectors(
    const std::vector<mojom::OrderExtractionSelectorInfoPtr>&
        mojom_order_extraction_selectors) {
  std::vector<mojom::OrderSelectorInfoPtr> mojom_order_selectors;
  mojom_order_selectors.reserve(mojom_order_extraction_selectors.size());

  for (const auto& mojom_order_extraction_selector :
       mojom_order_extraction_selectors) {
    mojom::OrderSelectorInfoPtr mojom_order_selector =
        mojom::OrderSelectorInfo::New();
    mojom_order_selector->selector = mojom_order_extraction_selector->selector;
    mojom_order_selector->attribute_name =
        mojom_order_extraction_selector->attribute_name;
    mojom_order_selector->pattern = mojom_order_extraction_selector->pattern;
    mojom_order_selector->validation_pattern =
        mojom_order_extraction_selector->validation_pattern;
    mojom_order_selectors.push_back(std::move(mojom_order_selector));
  }

  return mojom_order_selectors;
}

}  // namespace

mojom::OrderPatternsInfoPtr ToMojomOrderPatterns(
    const mojom::OrderExtractionPatternsInfo& mojom_order_extraction_patterns) {
  mojom::OrderPatternsInfoPtr mojom_order_patterns =
      mojom::OrderPatternsInfo::New();

  if (mojom_order_extraction_patterns.id) {
    mojom_order_patterns->id_selectors =
        ToMojomOrderSelectors(mojom_order_extraction_patterns.id->selectors);
    mojom_order_patterns->id_regexes =
        mojom_order_extraction_patterns.id->regexes;
    mojom_order_patterns->id_max_combined_text_length_to_search =
        mojom_order_extraction_patterns.id->max_combined_text_length_to_search;
    mojom_order_patterns->id_number_of_descendant_levels_to_search =
        mojom_order_extraction_patterns.id
            ->number_of_descendant_levels_to_search;
    mojom_order_patterns->id_validation_pattern =
        mojom_order_extraction_patterns.id->validation_pattern;
  }

  if (mojom_order_extraction_patterns.value) {
    mojom_order_patterns->value_selectors =
        ToMojomOrderSelectors(mojom_order_extraction_patterns.value->selectors);
    mojom_order_patterns->value_currency_regexes =
        mojom_order_extraction_patterns.value->currency_regexes;
    mojom_order_patterns->value_keyword_regexes =
        mojom_order_extraction_patterns.value->keyword_regexes;
    mojom_order_patterns->number_of_ancestor_levels_to_search =
        mojom_order_extraction_patterns.value
            ->number_of_ancestor_levels_to_search;
  }

  return mojom_order_patterns;
}

}  // namespace brave_ads
