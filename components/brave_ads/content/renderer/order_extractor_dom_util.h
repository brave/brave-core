/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_EXTRACTOR_DOM_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_EXTRACTOR_DOM_UTIL_H_

#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom-forward.h"

namespace blink {
class WebDocument;
class WebNode;
}  // namespace blink

namespace brave_ads {

// Walked manually because these patterns use RE2 syntax, which
// `FindAllTextNodesMatchingRegex` does not accept.
std::vector<blink::WebNode> CollectTextNodes(
    const blink::WebNode& root_web_node);

// Returns the resolved text/attribute value for the first selector in
// `mojom_selectors` that matches an element on `web_document`, or nullopt.
std::optional<std::string> ResolveSelector(
    const blink::WebDocument& web_document,
    const std::vector<mojom::OrderSelectorInfoPtr>& mojom_selectors);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_EXTRACTOR_DOM_UTIL_H_
