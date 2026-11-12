/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_SELECTOR_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_SELECTOR_INFO_H_

#include <optional>
#include <string>
#include <vector>

namespace brave_ads {

// Mirrors the renderer-facing `mojom::OrderSelectorInfo`, kept as a separate
// plain struct since `core/internal` cannot depend on `content/common/mojom`.
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

  // Attribute to read from the matched element. If not given, reads the
  // element's `textContent` instead.
  std::optional<std::string> attribute_name;

  // Regex applied to the resolved text/attribute value, e.g. to isolate an
  // identifier embedded in a `<script>` element's full source text rather
  // than a clean, already-isolated value. Uses the first capture group if
  // the regex has one, otherwise the whole match. If not given, the
  // resolved value is used as-is.
  std::optional<std::string> pattern;
};

using OrderExtractionSelectors = std::vector<OrderExtractionSelectorInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_SELECTOR_INFO_H_
