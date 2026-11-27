/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_VALUE_EXTRACTOR_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_VALUE_EXTRACTOR_UTIL_H_

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom-forward.h"
#include "brave/components/brave_ads/content/renderer/order_value_match_info.h"

namespace blink {
class WebDocument;
class WebNode;
}  // namespace blink

namespace re2 {
class RE2;
}  // namespace re2

namespace brave_ads {

// Returns the first currency code whose regex matches `text`, or
// `std::nullopt` if none do.
std::optional<std::string> MaybeMatchCurrency(
    const std::string& text,
    const base::flat_map<std::string, std::string>& currency_regexes);

// Returns the first order value match found using the provided selectors
// and currency regexes, or `std::nullopt` if none are found.
std::optional<OrderValueMatchInfo> MaybeResolveValueSelector(
    const blink::WebDocument& web_document,
    const std::vector<mojom::OrderSelectorInfoPtr>& mojom_selectors,
    const base::flat_map<std::string, std::string>& currency_regexes);

// Returns a single non-capturing regex that matches any of the input
// patterns, or nullptr if none compile.
std::unique_ptr<re2::RE2> JoinRegexes(base::span<const std::string> regexes);

// Returns the first order value match found using the keyword heuristic, or
// `std::nullopt` if none are found.
std::optional<OrderValueMatchInfo> MaybeFindLabeledTextNodeValue(
    base::span<const blink::WebNode> text_nodes,
    const base::flat_map<std::string, std::string>& currency_regexes,
    const re2::RE2& keyword_regex,
    size_t number_of_ancestor_levels_to_search);

// Returns the bare number extracted from `text`, or `std::nullopt` if none
// is found.
std::optional<std::string> MaybeExtractBareNumber(const std::string& text);

// Returns the first order value match found using the order patterns, or
// `std::nullopt` if none are found.
std::optional<OrderValueMatchInfo> MaybeExtractOrderValue(
    const blink::WebDocument& web_document,
    base::span<const blink::WebNode> text_nodes,
    const mojom::OrderPatternsInfo& mojom_order_patterns);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_VALUE_EXTRACTOR_UTIL_H_
