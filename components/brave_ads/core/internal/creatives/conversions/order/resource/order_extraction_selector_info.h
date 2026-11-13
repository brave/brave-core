/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_RESOURCE_ORDER_EXTRACTION_SELECTOR_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_RESOURCE_ORDER_EXTRACTION_SELECTOR_INFO_H_

#include <optional>
#include <string>
#include <vector>

namespace brave_ads {

struct OrderExtractionSelectorInfo final {
  OrderExtractionSelectorInfo();

  OrderExtractionSelectorInfo(const OrderExtractionSelectorInfo&);
  OrderExtractionSelectorInfo& operator=(const OrderExtractionSelectorInfo&);

  OrderExtractionSelectorInfo(OrderExtractionSelectorInfo&&) noexcept;
  OrderExtractionSelectorInfo& operator=(
      OrderExtractionSelectorInfo&&) noexcept;

  ~OrderExtractionSelectorInfo();

  bool operator==(const OrderExtractionSelectorInfo&) const = default;

  std::string selector;

  std::optional<std::string> attribute_name;

  std::optional<std::string> pattern;

  std::optional<std::string> validation_pattern;
};

using OrderExtractionSelectors = std::vector<OrderExtractionSelectorInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_RESOURCE_ORDER_EXTRACTION_SELECTOR_INFO_H_
