/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_RESOURCE_COMPONENT_ORDER_ID_EXTRACTION_PATTERNS_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_RESOURCE_COMPONENT_ORDER_ID_EXTRACTION_PATTERNS_INFO_H_

#include <string>

#include "base/containers/flat_map.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_page_type.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_selector_info.h"

namespace brave_ads {

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

  // Per-site CSS selectors for order ID extraction, keyed by site and page
  // type.
  base::flat_map<
      /*site*/ std::string,
      base::flat_map<OrderExtractionPageType, OrderExtractionSelectors>>
      selectors;

  // Single-step label+value regex, keyed by language.
  base::flat_map</*language*/ std::string, /*regex*/ std::string> regexes;

  // Maximum combined element text length, in UTF-8 bytes, to search for order
  // ID, used when the current site has no override.
  int default_max_combined_text_length_to_search = 0;

  // Per-site override for `default_max_combined_text_length_to_search`.
  base::flat_map</*site*/ std::string, /*bytes*/ int>
      max_combined_text_length_to_search_overrides;

  // Maximum number of descendant levels to search for order ID, used when the
  // current site has no override.
  int default_number_of_descendant_levels_to_search = 0;

  // Per-site override for `default_number_of_descendant_levels_to_search`.
  base::flat_map</*site*/ std::string, /*levels*/ int>
      number_of_descendant_levels_to_search_overrides;

  // Default regex pattern to validate extracted order ID, used when the
  // current site has no override.
  std::string default_validation_pattern;

  // Per-site override for `default_validation_pattern`.
  base::flat_map</*site*/ std::string, /*pattern*/ std::string>
      validation_pattern_overrides;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_RESOURCE_COMPONENT_ORDER_ID_EXTRACTION_PATTERNS_INFO_H_
