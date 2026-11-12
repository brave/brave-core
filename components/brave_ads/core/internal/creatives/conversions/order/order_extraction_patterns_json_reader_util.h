/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_JSON_READER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_JSON_READER_UTIL_H_

#include <optional>
#include <string>
#include <string_view>

#include "base/containers/flat_map.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_page_type.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_selector_info.h"

namespace base {
class DictValue;
class ListValue;
}  // namespace base

namespace brave_ads::json::reader {

std::optional<OrderExtractionPageType> ParsePageType(
    std::string_view page_type);

base::flat_map<std::string, std::string> ParseStringMap(
    const base::DictValue* dict);

std::optional<OrderExtractionSelectorInfo> ParseSelector(
    const base::DictValue& dict);

OrderExtractionSelectors ParseSelectorList(const base::ListValue& list);

base::flat_map<
    std::string,
    base::flat_map<OrderExtractionPageType, OrderExtractionSelectors>>
ParseSelectorsByDomain(const base::DictValue* dict);

}  // namespace brave_ads::json::reader

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_JSON_READER_UTIL_H_
