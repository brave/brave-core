/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_JSON_READER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_JSON_READER_H_

namespace base {
class DictValue;
}  // namespace base

namespace brave_ads {

struct OrderExtractionPatternsInfo;

namespace json::reader {

// An absent or malformed section is left empty rather than failing the
// whole read, so a stale/partial schema degrades gracefully.
OrderExtractionPatternsInfo ReadOrderPatterns(
    const base::DictValue& order_dict);

}  // namespace json::reader

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_JSON_READER_H_
