/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_ID_EXTRACTOR_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_ID_EXTRACTOR_UTIL_H_

#include <optional>
#include <set>
#include <string>

#include "base/containers/span.h"

namespace blink {
class WebDocument;
class WebElement;
class WebNode;
}  // namespace blink

namespace re2 {
class RE2;
}  // namespace re2

namespace brave_ads {

// Recurses into `web_element`'s children, skipping `<script>`/`<style>`,
// and returns its combined text. A child's own match (tracked locally to
// this subtree, so unrelated branches elsewhere are unaffected) stops its
// ancestors from being tested too, so the smallest containing element
// wins and a large container cannot shadow an already-found match.
std::string FindElementTextMatch(const blink::WebElement& web_element,
                                 const re2::RE2& regex,
                                 std::set<std::string>& captured_ids,
                                 bool& matched);

// For sites that split the label and ID across sibling elements, e.g.
// `<span>Confirmation number:</span><span>123</span>`, where no single
// text node contains both.
std::optional<std::string> FindCapturedIdInElementText(
    const blink::WebDocument& web_document,
    const re2::RE2& regex);

// Returns the sole captured ID matched by `regex` across `text_nodes`, or
// nullopt if zero or more than one distinct value was captured.
std::optional<std::string> FindCapturedIdInTextNodes(
    base::span<const blink::WebNode> text_nodes,
    const re2::RE2& regex);

// Each `id_regexes` entry already captures the bare ID, so unlike `value`
// no ancestor search is needed; multiple distinct captures are unreliable.
// Falls back to `FindCapturedIdInElementText` only when the label and ID
// share no single text node, since it costs a second, element-wide walk.
std::optional<std::string> FindCapturedId(
    const blink::WebDocument& web_document,
    base::span<const blink::WebNode> text_nodes,
    base::span<const std::string> id_regexes);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_ID_EXTRACTOR_UTIL_H_
