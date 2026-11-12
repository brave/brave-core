/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_VALUE_EXTRACTION_PATTERNS_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_VALUE_EXTRACTION_PATTERNS_INFO_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_selector_info.h"

namespace brave_ads {

struct OrderValueExtractionPatternsInfo final {
  OrderValueExtractionPatternsInfo();

  OrderValueExtractionPatternsInfo(const OrderValueExtractionPatternsInfo&);
  OrderValueExtractionPatternsInfo& operator=(
      const OrderValueExtractionPatternsInfo&);

  OrderValueExtractionPatternsInfo(OrderValueExtractionPatternsInfo&&) noexcept;
  OrderValueExtractionPatternsInfo& operator=(
      OrderValueExtractionPatternsInfo&&) noexcept;

  ~OrderValueExtractionPatternsInfo();

  // Tried before the regexes below, the generic fallback.
  OrderExtractionSelectors selectors;

  // Matches a formatted price for the given currency, keyed by currency
  // code so a matching currency's identity survives into `OrderInfo`.
  base::flat_map</*currency*/ std::string, /*regex*/ std::string>
      currency_regexes;

  // Ordered by priority, the detected language's pattern is tried first,
  // with the rest as fallback.
  std::vector<std::string> keyword_regexes;

  uint32_t number_of_ancestor_levels_to_search = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_VALUE_EXTRACTION_PATTERNS_INFO_H_
