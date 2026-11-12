/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_mojom_util.h"

#include <utility>

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_selector_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_id_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_value_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

namespace {

std::vector<mojom::OrderExtractionSelectorInfoPtr>
ToMojomOrderExtractionSelectors(
    OrderExtractionSelectors order_extraction_selectors) {
  std::vector<mojom::OrderExtractionSelectorInfoPtr> mojom_selectors;
  mojom_selectors.reserve(order_extraction_selectors.size());

  for (auto& order_extraction_selector : order_extraction_selectors) {
    mojom::OrderExtractionSelectorInfoPtr mojom_selector =
        mojom::OrderExtractionSelectorInfo::New();
    mojom_selector->selector = std::move(order_extraction_selector.selector);
    mojom_selector->attribute_name =
        std::move(order_extraction_selector.attribute_name);
    mojom_selector->pattern = std::move(order_extraction_selector.pattern);
    mojom_selectors.push_back(std::move(mojom_selector));
  }

  return mojom_selectors;
}

}  // namespace

mojom::OrderExtractionPatternsInfoPtr ToMojomOrderExtractionPatterns(
    std::optional<OrderIdExtractionPatternsInfo> id,
    std::optional<OrderValueExtractionPatternsInfo> value) {
  if (!id && !value) {
    return {};
  }

  mojom::OrderExtractionPatternsInfoPtr mojom_patterns =
      mojom::OrderExtractionPatternsInfo::New();

  if (id) {
    mojom::OrderIdExtractionPatternsInfoPtr mojom_id =
        mojom::OrderIdExtractionPatternsInfo::New();
    mojom_id->selectors =
        ToMojomOrderExtractionSelectors(std::move(id->selectors));
    mojom_id->regexes = std::move(id->regexes);
    mojom_patterns->id = std::move(mojom_id);
  }

  if (value) {
    mojom::OrderValueExtractionPatternsInfoPtr mojom_value =
        mojom::OrderValueExtractionPatternsInfo::New();
    mojom_value->selectors =
        ToMojomOrderExtractionSelectors(std::move(value->selectors));
    mojom_value->currency_regexes = std::move(value->currency_regexes);
    mojom_value->keyword_regexes = std::move(value->keyword_regexes);
    mojom_value->number_of_ancestor_levels_to_search =
        value->number_of_ancestor_levels_to_search;
    mojom_patterns->value = std::move(mojom_value);
  }

  return mojom_patterns;
}

}  // namespace brave_ads
