/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_RESOURCE_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_RESOURCE_INFO_H_

#include <optional>

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_info.h"

namespace base {
class DictValue;
}  // namespace base

namespace brave_ads {

struct OrderExtractionPatternsResourceInfo final {
  OrderExtractionPatternsResourceInfo();

  OrderExtractionPatternsResourceInfo(
      const OrderExtractionPatternsResourceInfo&) = delete;
  OrderExtractionPatternsResourceInfo& operator=(
      const OrderExtractionPatternsResourceInfo&) = delete;

  OrderExtractionPatternsResourceInfo(
      OrderExtractionPatternsResourceInfo&&) noexcept;
  OrderExtractionPatternsResourceInfo& operator=(
      OrderExtractionPatternsResourceInfo&&) noexcept;

  ~OrderExtractionPatternsResourceInfo();

  static std::optional<OrderExtractionPatternsResourceInfo> MaybeFromDict(
      base::DictValue dict);

  std::optional<int> version;
  OrderExtractionPatternsInfo patterns;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_RESOURCE_INFO_H_
