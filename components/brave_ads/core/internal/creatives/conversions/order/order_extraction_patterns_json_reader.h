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

// Returns the parsed patterns, or `std::nullopt` if any `id`/`value`/`sites`
// entry is malformed.
std::optional<OrderExtractionPatternsInfo> MaybeReadOrderPatterns(
    const base::DictValue& order_dict);

// Returns the parsed resource, or `std::nullopt` if `conversions.order` is
// present but malformed; a missing `conversions.order` gives empty patterns.
// `version` is copied as-is and not validated, so the caller must check it.
std::optional<OrderExtractionPatternsResourceInfo>
MaybeReadOrderExtractionPatternsResource(const base::DictValue& dict);

}  // namespace json::reader

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_JSON_READER_H_
