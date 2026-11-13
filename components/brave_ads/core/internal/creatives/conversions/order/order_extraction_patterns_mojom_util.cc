/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_mojom_util.h"

#include <utility>

#include "base/numerics/safe_conversions.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_selector_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_id_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_value_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

namespace {

std::vector<mojom::OrderExtractionSelectorInfoPtr>
ToMojomOrderExtractionSelectors(
    OrderExtractionSelectors order_extraction_selectors) {
  std::vector<mojom::OrderExtractionSelectorInfoPtr>
      mojom_order_extraction_selectors;
  mojom_order_extraction_selectors.reserve(order_extraction_selectors.size());

  for (auto& order_extraction_selector : order_extraction_selectors) {
    mojom::OrderExtractionSelectorInfoPtr mojom_order_extraction_selector =
        mojom::OrderExtractionSelectorInfo::New();
    mojom_order_extraction_selector->selector =
        std::move(order_extraction_selector.selector);
    mojom_order_extraction_selector->attribute_name =
        std::move(order_extraction_selector.attribute_name);
    mojom_order_extraction_selector->pattern =
        std::move(order_extraction_selector.pattern);
    mojom_order_extraction_selector->validation_pattern =
        std::move(order_extraction_selector.validation_pattern);
    mojom_order_extraction_selectors.push_back(
        std::move(mojom_order_extraction_selector));
  }

  return mojom_order_extraction_selectors;
}

}  // namespace

mojom::OrderExtractionPatternsInfoPtr ToMojomOrderExtractionPatterns(
    OrderIdExtractionPatternsInfo id_extraction_patterns,
    bool should_extract_id,
    OrderValueExtractionPatternsInfo value_extraction_patterns,
    bool should_extract_value) {
  mojom::OrderExtractionPatternsInfoPtr mojom_order_extraction_patterns =
      mojom::OrderExtractionPatternsInfo::New();

  mojom::OrderIdExtractionPatternsInfoPtr mojom_order_id_extraction_patterns =
      mojom::OrderIdExtractionPatternsInfo::New();
  mojom_order_id_extraction_patterns->selectors =
      ToMojomOrderExtractionSelectors(
          std::move(id_extraction_patterns.selectors));
  mojom_order_id_extraction_patterns->regexes =
      std::move(id_extraction_patterns.regexes);
  mojom_order_id_extraction_patterns->max_combined_text_length_to_search =
      base::saturated_cast<uint32_t>(
          id_extraction_patterns.max_combined_text_length_to_search);
  mojom_order_id_extraction_patterns->number_of_descendant_levels_to_search =
      base::saturated_cast<uint32_t>(
          id_extraction_patterns.number_of_descendant_levels_to_search);
  mojom_order_id_extraction_patterns->validation_pattern =
      std::move(id_extraction_patterns.validation_pattern);
  mojom_order_extraction_patterns->id =
      std::move(mojom_order_id_extraction_patterns);
  mojom_order_extraction_patterns->should_extract_id = should_extract_id;

  mojom::OrderValueExtractionPatternsInfoPtr
      mojom_order_value_extraction_patterns =
          mojom::OrderValueExtractionPatternsInfo::New();
  mojom_order_value_extraction_patterns->selectors =
      ToMojomOrderExtractionSelectors(
          std::move(value_extraction_patterns.selectors));
  mojom_order_value_extraction_patterns->currency_regexes =
      std::move(value_extraction_patterns.currency_regexes);
  mojom_order_value_extraction_patterns->keyword_regexes =
      std::move(value_extraction_patterns.keyword_regexes);
  mojom_order_value_extraction_patterns->number_of_ancestor_levels_to_search =
      base::saturated_cast<uint32_t>(
          value_extraction_patterns.number_of_ancestor_levels_to_search);
  mojom_order_extraction_patterns->value =
      std::move(mojom_order_value_extraction_patterns);
  mojom_order_extraction_patterns->should_extract_value = should_extract_value;

  return mojom_order_extraction_patterns;
}

}  // namespace brave_ads
