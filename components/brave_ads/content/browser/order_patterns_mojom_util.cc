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
    const std::vector<mojom::OrderExtractionSelectorInfoPtr>& mojom_selectors) {
  std::vector<mojom::OrderSelectorInfoPtr> converted_mojom_selectors;
  converted_mojom_selectors.reserve(mojom_selectors.size());

  for (const auto& mojom_selector : mojom_selectors) {
    mojom::OrderSelectorInfoPtr converted_mojom_selector =
        mojom::OrderSelectorInfo::New();
    converted_mojom_selector->selector = mojom_selector->selector;
    converted_mojom_selector->attribute_name = mojom_selector->attribute_name;
    converted_mojom_selector->pattern = mojom_selector->pattern;
    converted_mojom_selectors.push_back(std::move(converted_mojom_selector));
  }

  return converted_mojom_selectors;
}

}  // namespace

mojom::OrderPatternsInfoPtr ToMojomOrderPatterns(
    const mojom::OrderExtractionPatternsInfo& mojom_order_patterns) {
  mojom::OrderPatternsInfoPtr converted_mojom_order_patterns =
      mojom::OrderPatternsInfo::New();

  if (mojom_order_patterns.id) {
    converted_mojom_order_patterns->id_selectors =
        ToMojomOrderSelectors(mojom_order_patterns.id->selectors);
    converted_mojom_order_patterns->id_regexes =
        mojom_order_patterns.id->regexes;
  }

  if (mojom_order_patterns.value) {
    converted_mojom_order_patterns->value_selectors =
        ToMojomOrderSelectors(mojom_order_patterns.value->selectors);
    converted_mojom_order_patterns->value_currency_regexes =
        mojom_order_patterns.value->currency_regexes;
    converted_mojom_order_patterns->value_keyword_regexes =
        mojom_order_patterns.value->keyword_regexes;
    converted_mojom_order_patterns->number_of_ancestor_levels_to_search =
        mojom_order_patterns.value->number_of_ancestor_levels_to_search;
  }

  return converted_mojom_order_patterns;
}

}  // namespace brave_ads
