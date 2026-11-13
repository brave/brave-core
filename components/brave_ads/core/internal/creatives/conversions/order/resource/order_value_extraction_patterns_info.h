/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_RESOURCE_ORDER_VALUE_EXTRACTION_PATTERNS_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_RESOURCE_ORDER_VALUE_EXTRACTION_PATTERNS_INFO_H_

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_selector_info.h"

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

  OrderExtractionSelectors selectors;

  base::flat_map</*currency*/ std::string, /*regex*/ std::string>
      currency_regexes;

  std::vector<std::string> keyword_regexes;

  int number_of_ancestor_levels_to_search = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_RESOURCE_ORDER_VALUE_EXTRACTION_PATTERNS_INFO_H_
