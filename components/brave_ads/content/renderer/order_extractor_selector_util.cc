/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/renderer/order_extractor_selector_util.h"

#include <cstddef>
#include <utility>
#include <vector>

#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom.h"
#include "brave/components/brave_ads/content/renderer/order_selector_match_info.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_node.h"
#include "third_party/re2/src/re2/re2.h"

namespace brave_ads {

std::optional<std::string> MaybeExtractConcatenatedGroups(
    std::string_view text,
    const re2::RE2& regex) {
  const int number_of_capturing_groups = regex.NumberOfCapturingGroups();
  if (number_of_capturing_groups <= 0) {
    return std::nullopt;
  }

  std::vector<std::string> groups(
      static_cast<size_t>(number_of_capturing_groups));
  std::vector<re2::RE2::Arg> args;
  args.reserve(groups.size());
  for (std::string& group : groups) {
    args.emplace_back(&group);
  }
  std::vector<const re2::RE2::Arg*> arg_ptrs;
  arg_ptrs.reserve(args.size());
  for (const re2::RE2::Arg& arg : args) {
    arg_ptrs.push_back(&arg);
  }

  if (!re2::RE2::PartialMatchN(text, regex, arg_ptrs.data(),
                               number_of_capturing_groups)) {
    return std::nullopt;
  }

  std::string concatenated;
  for (const std::string& group : groups) {
    concatenated += group;
  }
  if (concatenated.empty()) {
    return std::nullopt;
  }
  return concatenated;
}

namespace {

// Returns the resolved value and validation pattern, or `std::nullopt` if
// the element has no text or `pattern` is set but does not match.
std::optional<OrderSelectorMatchInfo> MaybeResolveSelectorValue(
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
    return OrderSelectorMatchInfo{
        .value = value,
        .validation_pattern = mojom_selector.validation_pattern};
  }

  const re2::RE2 regex(*mojom_selector.pattern, re2::RE2::Quiet);
  if (!regex.ok()) {
    return std::nullopt;
  }
  std::optional<std::string> match =
      MaybeExtractConcatenatedGroups(value, regex);
  if (!match) {
    return std::nullopt;
  }
  return OrderSelectorMatchInfo{
      .value = std::move(*match),
      .validation_pattern = mojom_selector.validation_pattern};
}

// Returns true if the given web node is a `<script>` or `<style>` element.
bool IsScriptOrStyleElement(const blink::WebNode& web_node) {
  if (!web_node.IsElementNode()) {
    return false;
  }
  const blink::WebElement web_element = web_node.To<blink::WebElement>();
  return web_element.HasHTMLTagName("script") ||
         web_element.HasHTMLTagName("style");
}

// Returns true if the given web node duplicates visible text for assistive
// technology, e.g. a digit-by-digit accessibility rendering of a price.
bool IsAriaHiddenElement(const blink::WebNode& web_node) {
  if (!web_node.IsElementNode()) {
    return false;
  }
  const blink::WebElement web_element = web_node.To<blink::WebElement>();
  return web_element.GetAttribute(blink::WebString::FromUtf8("aria-hidden"))
             .Utf8() == "true";
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
    blink::WebNode next_web_node =
        (IsScriptOrStyleElement(web_node) || IsAriaHiddenElement(web_node))
            ? blink::WebNode()
            : web_node.FirstChild();
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

std::vector<OrderSelectorMatchInfo> CollectSelectorMatches(
    const blink::WebDocument& web_document,
    const std::vector<mojom::OrderSelectorInfoPtr>& mojom_selectors) {
  std::vector<OrderSelectorMatchInfo> matches;
  for (const auto& mojom_selector : mojom_selectors) {
    const std::vector<blink::WebElement> elements =
        web_document.QuerySelectorAll(
            blink::WebString::FromUtf8(mojom_selector->selector));
    for (const blink::WebElement& web_element : elements) {
      if (std::optional<OrderSelectorMatchInfo> match =
              MaybeResolveSelectorValue(web_element, *mojom_selector)) {
        matches.push_back(std::move(*match));
      }
    }
  }
  return matches;
}

}  // namespace brave_ads
