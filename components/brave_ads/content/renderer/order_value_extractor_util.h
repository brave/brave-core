/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_VALUE_EXTRACTOR_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_VALUE_EXTRACTOR_UTIL_H_

#include <cstdint>
#include <optional>
#include <string>

#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "brave/components/brave_ads/content/renderer/order_value_match_info.h"

namespace blink {
class WebNode;
}  // namespace blink

namespace re2 {
class RE2;
}  // namespace re2

namespace brave_ads {

// Only for boolean-match fields; never for `id_regexes`, whose capture
// groups would collide once joined.
re2::RE2 JoinRegexes(base::span<const std::string> regexes);

// Returns the currency code whose regex matches `text`, or nullopt if
// none do (e.g. a bare total with no currency marker in that same text);
// `OrderInfo.currency` is left unset rather than guessing in that case.
std::optional<std::string> MatchCurrency(
    const std::string& text,
    const base::flat_map<std::string, std::string>& currency_regexes);

// The matching text node's ancestor must have no other currency-regex
// match, so a label with two nearby prices is treated as ambiguous.
std::optional<OrderValueMatchInfo> FindLabeledTextNodeValue(
    base::span<const blink::WebNode> text_nodes,
    const base::flat_map<std::string, std::string>& currency_regexes,
    const re2::RE2& keyword_regex,
    uint32_t number_of_ancestor_levels_to_search);

// PII safety net, only digits/separators/a decimal separator ever leave as
// `value`; no match drops the candidate rather than guessing.
std::optional<std::string> ExtractBareNumber(const std::string& candidate);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_VALUE_EXTRACTOR_UTIL_H_
