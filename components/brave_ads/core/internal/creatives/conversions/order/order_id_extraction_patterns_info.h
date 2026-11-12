/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_ID_EXTRACTION_PATTERNS_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_ID_EXTRACTION_PATTERNS_INFO_H_

#include <string>
#include <vector>

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_selector_info.h"

namespace brave_ads {

struct OrderIdExtractionPatternsInfo final {
  OrderIdExtractionPatternsInfo();

  OrderIdExtractionPatternsInfo(const OrderIdExtractionPatternsInfo&);
  OrderIdExtractionPatternsInfo& operator=(
      const OrderIdExtractionPatternsInfo&);

  OrderIdExtractionPatternsInfo(OrderIdExtractionPatternsInfo&&) noexcept;
  OrderIdExtractionPatternsInfo& operator=(
      OrderIdExtractionPatternsInfo&&) noexcept;

  ~OrderIdExtractionPatternsInfo();

  // Tried before `regexes`, the generic fallback.
  OrderExtractionSelectors selectors;

  // Ordered by priority, the detected language's pattern is tried first,
  // with the rest as fallback.
  std::vector<std::string> regexes;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_ID_EXTRACTION_PATTERNS_INFO_H_
