/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_ID_EXTRACTOR_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_ID_EXTRACTOR_UTIL_H_

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "base/types/optional_ref.h"
#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom-forward.h"

namespace blink {
class WebDocument;
class WebNode;
}  // namespace blink

namespace re2 {
class RE2;
}  // namespace re2

namespace brave_ads {

// Returns the ID if it passes validation against the provided pattern, or
// `std::nullopt` otherwise.
std::optional<std::string> MaybeValidateId(
    std::string id,
    base::optional_ref<const std::string> validation_pattern);

// Returns the first valid ID resolved using the provided selectors, or
// `std::nullopt` if none are valid.
std::optional<std::string> MaybeResolveIdSelector(
    const blink::WebDocument& web_document,
    const std::vector<mojom::OrderSelectorInfoPtr>& mojom_selectors,
    base::optional_ref<const std::string> domain_validation_pattern);

// Returns the first ID captured by `regex` across `text_nodes` that passes
// validation, or `std::nullopt` if none are valid.
std::optional<std::string> MaybeFindCapturedIdInTextNodes(
    base::span<const blink::WebNode> text_nodes,
    const re2::RE2& regex,
    base::optional_ref<const std::string> validation_pattern);

// Returns the first ID captured by any of the provided regexes that passes
// validation, or `std::nullopt` if none are valid.
std::optional<std::string> MaybeFindCapturedIdUsingRegexes(
    const blink::WebDocument& web_document,
    base::span<const blink::WebNode> text_nodes,
    base::span<const std::string> id_regexes,
    size_t max_combined_text_length_to_search,
    size_t number_of_descendant_levels_to_search,
    base::optional_ref<const std::string> validation_pattern);

// Returns the order ID extracted from the document, either via selectors or
// regexes, or `std::nullopt` if none is found.
std::optional<std::string> MaybeExtractOrderId(
    const blink::WebDocument& web_document,
    base::span<const blink::WebNode> text_nodes,
    const mojom::OrderPatternsInfo& mojom_order_patterns);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_ID_EXTRACTOR_UTIL_H_
