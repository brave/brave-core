/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/renderer/order_extractor_dom_util.h"

#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_node.h"
#include "third_party/re2/src/re2/re2.h"

namespace brave_ads {

namespace {

// Resolves one already-matched element's text/attribute value, then applies
// `mojom_selector.pattern` if set. `pattern` must have exactly one capture
// group; `PartialMatch` only succeeds when the regex's capture-group count
// matches the number of output arguments given, so a 0- or 2-plus-group
// `pattern` never matches here, rather than matching something unintended.
std::optional<std::string> ResolveSelectorValue(
    const blink::WebElement& web_element,
    const mojom::OrderSelectorInfo& mojom_selector) {
  const blink::WebString web_string_value =
      mojom_selector.attribute_name
          ? web_element.GetAttribute(
                blink::WebString::FromUtf8(*mojom_selector.attribute_name))
          : web_element.TextContent();
  if (web_string_value.IsNull() || web_string_value.IsEmpty()) {
    return std::nullopt;
  }

  const std::string value = web_string_value.Utf8();
  if (!mojom_selector.pattern) {
    return value;
  }

  const re2::RE2 regex(*mojom_selector.pattern, re2::RE2::Quiet);
  std::string match;
  if (!regex.ok() || !re2::RE2::PartialMatch(value, regex, &match) ||
      match.empty()) {
    return std::nullopt;
  }
  return match;
}

}  // namespace

std::vector<blink::WebNode> CollectTextNodes(
    const blink::WebNode& root_web_node) {
  std::vector<blink::WebNode> text_nodes;
  blink::WebNode web_node = root_web_node.FirstChild();
  while (!web_node.IsNull()) {
    if (web_node.IsTextNode()) {
      text_nodes.push_back(web_node);
    }
    blink::WebNode next_web_node = web_node.FirstChild();
    while (next_web_node.IsNull() && web_node != root_web_node) {
      next_web_node = web_node.NextSibling();
      if (next_web_node.IsNull()) {
        web_node = web_node.ParentNode();
      }
    }
    web_node = next_web_node;
  }
  return text_nodes;
}

std::optional<std::string> ResolveSelector(
    const blink::WebDocument& web_document,
    const std::vector<mojom::OrderSelectorInfoPtr>& mojom_selectors) {
  for (const auto& mojom_selector : mojom_selectors) {
    // A selector can match several elements (e.g. every inline `<script>`
    // on the page), and only one of them may actually hold the value, so
    // every match is tried before moving on to the next selector.
    for (const blink::WebElement& web_element : web_document.QuerySelectorAll(
             blink::WebString::FromUtf8(mojom_selector->selector))) {
      if (std::optional<std::string> value =
              ResolveSelectorValue(web_element, *mojom_selector)) {
        return value;
      }
    }
  }
  return std::nullopt;
}

}  // namespace brave_ads
