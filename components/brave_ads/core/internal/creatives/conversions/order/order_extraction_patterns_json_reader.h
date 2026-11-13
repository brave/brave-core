/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_JSON_READER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_JSON_READER_H_

#include <optional>

namespace base {
class DictValue;
}  // namespace base

namespace brave_ads {

struct OrderExtractionPatternsInfo;
struct OrderExtractionPatternsResourceInfo;

namespace json::reader {

std::optional<OrderExtractionPatternsInfo> MaybeReadOrderPatterns(
    const base::DictValue& order_dict);

std::optional<OrderExtractionPatternsResourceInfo>
MaybeReadOrderExtractionPatternsResource(const base::DictValue& dict);

}  // namespace json::reader

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_JSON_READER_H_
