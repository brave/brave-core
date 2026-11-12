/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_COMPONENT_ORDER_VALUE_EXTRACTION_PATTERNS_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_COMPONENT_ORDER_VALUE_EXTRACTION_PATTERNS_INFO_H_

#include <cstdint>
#include <string>

#include "base/containers/flat_map.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_page_type.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_selector_info.h"

namespace brave_ads {

// Parsed from the `conversions.order.value` schema delivered on the
// Sponsored Images Component.
struct ComponentOrderValueExtractionPatternsInfo final {
  ComponentOrderValueExtractionPatternsInfo();

  ComponentOrderValueExtractionPatternsInfo(
      const ComponentOrderValueExtractionPatternsInfo&);
  ComponentOrderValueExtractionPatternsInfo& operator=(
      const ComponentOrderValueExtractionPatternsInfo&);

  ComponentOrderValueExtractionPatternsInfo(
      ComponentOrderValueExtractionPatternsInfo&&) noexcept;
  ComponentOrderValueExtractionPatternsInfo& operator=(
      ComponentOrderValueExtractionPatternsInfo&&) noexcept;

  ~ComponentOrderValueExtractionPatternsInfo();

  bool operator==(const ComponentOrderValueExtractionPatternsInfo&) const =
      default;

  // A per-site CSS selector that, when configured for the current domain
  // and page type (`checkout` or `thankYou`), is tried before the regexes
  // below.
  base::flat_map<
      /*domain*/ std::string,
      base::flat_map<OrderExtractionPageType, OrderExtractionSelectors>>
      selectors;

  // Matches a formatted price, like "4,98 €", for the given currency.
  base::flat_map</*currency*/ std::string, /*regex*/ std::string>
      currency_regexes;

  // Matches a label like "Total" or "Gesamt" next to the price, not the
  // price itself, keyed by language.
  base::flat_map</*language*/ std::string, /*regex*/ std::string>
      keyword_regexes;

  // Ancestor levels above a matched keyword label to search for a sibling
  // matching a `currency_regexes` entry.
  uint32_t number_of_ancestor_levels_to_search = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_COMPONENT_ORDER_VALUE_EXTRACTION_PATTERNS_INFO_H_
