/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_COMPONENT_ORDER_ID_EXTRACTION_PATTERNS_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_COMPONENT_ORDER_ID_EXTRACTION_PATTERNS_INFO_H_

#include <string>

#include "base/containers/flat_map.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_page_type.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_selector_info.h"

namespace brave_ads {

// Parsed from the `conversions.order.id` schema delivered on the Sponsored
// Images Component.
struct ComponentOrderIdExtractionPatternsInfo final {
  ComponentOrderIdExtractionPatternsInfo();

  ComponentOrderIdExtractionPatternsInfo(
      const ComponentOrderIdExtractionPatternsInfo&);
  ComponentOrderIdExtractionPatternsInfo& operator=(
      const ComponentOrderIdExtractionPatternsInfo&);

  ComponentOrderIdExtractionPatternsInfo(
      ComponentOrderIdExtractionPatternsInfo&&) noexcept;
  ComponentOrderIdExtractionPatternsInfo& operator=(
      ComponentOrderIdExtractionPatternsInfo&&) noexcept;

  ~ComponentOrderIdExtractionPatternsInfo();

  bool operator==(const ComponentOrderIdExtractionPatternsInfo&) const =
      default;

  // A per-site CSS selector that, when configured for the current domain
  // and page type (`checkout` or `thankYou`), is tried before `regexes`.
  base::flat_map<
      /*domain*/ std::string,
      base::flat_map<OrderExtractionPageType, OrderExtractionSelectors>>
      selectors;

  // Documents what `regexes` matches against. Currently always
  // "elementText" (each candidate text node's own content, no ancestor
  // search), reserved for a future second mode.
  std::string source;

  // Single-step label+value regex, keyed by language.
  base::flat_map</*language*/ std::string, /*regex*/ std::string> regexes;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_COMPONENT_ORDER_ID_EXTRACTION_PATTERNS_INFO_H_
