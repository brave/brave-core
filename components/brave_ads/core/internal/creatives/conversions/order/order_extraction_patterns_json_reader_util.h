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
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_page_type.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_selector_info.h"

namespace base {
class DictValue;
class ListValue;
class Value;
}  // namespace base

namespace brave_ads::json::reader {

// Returns the matching page type, or `std::nullopt` if `page_type` is not
// recognized.
std::optional<OrderExtractionPageType> MaybeParsePageType(
    std::string_view page_type);

// Returns the parsed selector, or `std::nullopt` if `selector` is missing,
// or a present `attributeName`/`validation.pattern` is empty or invalid.
std::optional<OrderExtractionSelectorInfo> MaybeParseSelector(
    const base::DictValue& dict);

// Returns the parsed selectors, or `std::nullopt` unless every entry is an
// object `MaybeParseSelector` accepts.
std::optional<OrderExtractionSelectors> MaybeParseSelectorList(
    const base::ListValue& list);

// Returns selectors grouped by `checkout`/`thankYou` key, ignoring keys such
// as `validation`; empty if none match, `std::nullopt` on a malformed entry.
std::optional<base::flat_map<OrderExtractionPageType, OrderExtractionSelectors>>
MaybeParseSelectorsByPageType(const base::DictValue& dict);

// Returns the parsed int, or `std::nullopt` if `value` is not an int or is
// negative. The caller already checked `value` is present.
std::optional<int> MaybeParseNonNegativeInt(const base::Value& value);

// Returns the pattern, or `std::nullopt` if `value` is not a string, empty,
// or fails to compile as RE2; the caller already confirmed `value` exists.
std::optional<std::string> MaybeParseValidationPattern(
    const base::Value& value);

// Returns entries that compile as RE2, dropping the rest; `std::nullopt` if
// any entry is a non-string, or every entry fails to compile.
std::optional<base::flat_map<std::string, std::string>>
MaybeParseCompiledRegexes(const base::Value& value);

}  // namespace brave_ads::json::reader

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_JSON_READER_UTIL_H_
