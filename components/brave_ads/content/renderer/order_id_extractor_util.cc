/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/renderer/order_id_extractor_util.h"

#include <optional>
#include <utility>

#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_node.h"
#include "third_party/re2/src/re2/re2.h"

namespace brave_ads {

std::string FindElementTextMatch(const blink::WebElement& web_element,
                                 const re2::RE2& regex,
                                 std::set<std::string>& captured_ids,
                                 bool& matched) {
  std::string text;
  bool child_matched = false;
  for (blink::WebNode child = web_element.FirstChild(); !child.IsNull();
       child = child.NextSibling()) {
    if (child.IsTextNode()) {
      text += child.NodeValue().Utf8();
      continue;
    }
    if (!child.IsElementNode()) {
      continue;
    }
    const blink::WebElement child_element = child.To<blink::WebElement>();
    if (child_element.HasHTMLTagName("script") ||
        child_element.HasHTMLTagName("style")) {
      continue;
    }
    bool child_element_matched = false;
    text += FindElementTextMatch(child_element, regex, captured_ids,
                                 child_element_matched);
    child_matched = child_matched || child_element_matched;
  }

  if (!child_matched) {
    std::string id;
    if (re2::RE2::PartialMatch(text, regex, &id) && !id.empty()) {
      captured_ids.insert(std::move(id));
      child_matched = true;
    }
  }

  matched = child_matched;
  return text;
}

std::optional<std::string> FindCapturedIdInElementText(
    const blink::WebDocument& web_document,
    const re2::RE2& regex) {
  const blink::WebElement document_element = web_document.DocumentElement();
  if (document_element.IsNull()) {
    return std::nullopt;
  }

  std::set<std::string> captured_ids;
  bool matched = false;
  FindElementTextMatch(document_element, regex, captured_ids, matched);
  if (captured_ids.size() != 1) {
    return std::nullopt;
  }
  return *captured_ids.begin();
}

std::optional<std::string> FindCapturedIdInTextNodes(
    base::span<const blink::WebNode> text_nodes,
    const re2::RE2& regex) {
  std::set<std::string> captured_ids;
  for (const blink::WebNode& web_node : text_nodes) {
    std::string id;
    if (re2::RE2::PartialMatch(web_node.NodeValue().Utf8(), regex, &id) &&
        !id.empty()) {
      captured_ids.insert(std::move(id));
    }
  }
  if (captured_ids.size() != 1) {
    return std::nullopt;
  }
  return *captured_ids.begin();
}

std::optional<std::string> FindCapturedId(
    const blink::WebDocument& web_document,
    base::span<const blink::WebNode> text_nodes,
    base::span<const std::string> id_regexes) {
  for (const std::string& pattern : id_regexes) {
    const re2::RE2 regex(pattern, re2::RE2::Quiet);
    if (!regex.ok()) {
      continue;
    }

    if (std::optional<std::string> id =
            FindCapturedIdInTextNodes(text_nodes, regex)) {
      return id;
    }

    if (std::optional<std::string> id =
            FindCapturedIdInElementText(web_document, regex)) {
      return id;
    }
  }
  return std::nullopt;
}

}  // namespace brave_ads
