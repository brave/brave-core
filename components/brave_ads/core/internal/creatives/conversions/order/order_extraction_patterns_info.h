/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_INFO_H_

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/component_order_id_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/component_order_value_extraction_patterns_info.h"

namespace brave_ads {

// Parsed from the `conversions.order` schema delivered on the Sponsored
// Images Component; platform/IPC-agnostic.
struct OrderExtractionPatternsInfo final {
  OrderExtractionPatternsInfo();

  OrderExtractionPatternsInfo(const OrderExtractionPatternsInfo&);
  OrderExtractionPatternsInfo& operator=(const OrderExtractionPatternsInfo&);

  OrderExtractionPatternsInfo(OrderExtractionPatternsInfo&&) noexcept;
  OrderExtractionPatternsInfo& operator=(
      OrderExtractionPatternsInfo&&) noexcept;

  ~OrderExtractionPatternsInfo();

  ComponentOrderIdExtractionPatternsInfo id;
  ComponentOrderValueExtractionPatternsInfo value;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_INFO_H_
