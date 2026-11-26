/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_EXTRACTOR_SELECTOR_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_EXTRACTOR_SELECTOR_UTIL_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom-forward.h"
#include "brave/components/brave_ads/content/renderer/order_selector_match_info.h"

namespace blink {
class WebDocument;
class WebNode;
}  // namespace blink

namespace re2 {
class RE2;
}  // namespace re2

namespace brave_ads {

// Returns the concatenated capture groups if the regex matches, or
// `std::nullopt` otherwise.
std::optional<std::string> MaybeExtractConcatenatedGroups(
    std::string_view text,
    const re2::RE2& regex);

// Returns all text nodes under the given root web node, excluding those
// inside `<script>` or `<style>` elements.
std::vector<blink::WebNode> CollectTextNodes(
    const blink::WebNode& root_web_node);

// Returns all elements matching the given selectors and resolves their values
// according to the selectors.
std::vector<OrderSelectorMatchInfo> CollectSelectorMatches(
    const blink::WebDocument& web_document,
    const std::vector<mojom::OrderSelectorInfoPtr>& mojom_selectors);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_EXTRACTOR_SELECTOR_UTIL_H_
