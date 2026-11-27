/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/renderer/order_id_extractor_util.h"

#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "base/strings/string_util.h"
#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom.h"
#include "brave/components/brave_ads/content/renderer/order_extractor_selector_util.h"
#include "brave/components/brave_ads/content/renderer/order_selector_match_info.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_node.h"
#include "third_party/re2/src/re2/re2.h"

namespace brave_ads {

namespace {

struct ElementTextRangeInfo {
  size_t frame_id = 0;
  std::optional<size_t> parent_frame_id;
  size_t text_start = 0;
  size_t text_end = 0;
  bool buffer_was_truncated = false;
};

struct ElementTextWalkFrame {
  blink::WebNode next_child_web_node;
  size_t frame_id = 0;
  size_t text_start = 0;
};

// Returns the text ranges for all elements under the given web element, up to
// the specified descendant levels and text length.
std::vector<ElementTextRangeInfo> CollectElementTextRanges(
    const blink::WebElement& web_element,
    size_t max_combined_text_length_to_search,
    size_t number_of_descendant_levels_to_search,
    std::string& buffer) {
  std::vector<ElementTextRangeInfo> ranges;
  std::vector<ElementTextWalkFrame> element_frames;
  size_t next_frame_id = 0;
  element_frames.push_back({.next_child_web_node = web_element.FirstChild(),
                            .frame_id = next_frame_id++,
                            .text_start = 0});
  bool buffer_was_truncated = false;

  while (!element_frames.empty()) {
    // Get the current element frame from the top of the stack.
    ElementTextWalkFrame& element_frame = element_frames.back();

    if (element_frame.next_child_web_node.IsNull()) {
      ranges.push_back(
          {.frame_id = element_frame.frame_id,
           .parent_frame_id =
               element_frames.size() > 1
                   ? std::optional(
                         element_frames[element_frames.size() - 2].frame_id)
                   : std::nullopt,
           .text_start = element_frame.text_start,
           .text_end = buffer.size(),
           .buffer_was_truncated = buffer_was_truncated});
      element_frames.pop_back();
      continue;
    }

    const blink::WebNode child_web_node = element_frame.next_child_web_node;
    element_frame.next_child_web_node = child_web_node.NextSibling();

    if (buffer.size() >= max_combined_text_length_to_search) {
      buffer_was_truncated = true;
      continue;
    }

    if (child_web_node.IsTextNode()) {
      const std::string node_text = child_web_node.NodeValue().Utf8();
      const size_t remaining_byte_size =
          max_combined_text_length_to_search - buffer.size();
      std::string_view truncated_text =
          base::TruncateUTF8ToByteSize(node_text, remaining_byte_size);
      if (truncated_text.size() < node_text.size()) {
        buffer_was_truncated = true;
      }
      buffer += truncated_text;
      continue;
    }

    if (!child_web_node.IsElementNode() ||
        element_frames.size() >= number_of_descendant_levels_to_search) {
      continue;
    }

    const blink::WebElement child_element =
        child_web_node.To<blink::WebElement>();
    if (child_element.HasHTMLTagName("script") ||
        child_element.HasHTMLTagName("style")) {
      continue;
    }

    element_frames.push_back({.next_child_web_node = child_element.FirstChild(),
                              .frame_id = next_frame_id++,
                              .text_start = buffer.size()});
  }

  return ranges;
}

// Skips a range whose descendant already matched, so a parent element is
// not also credited for text a nested element already captured.
void MatchElementTextRanges(const std::vector<ElementTextRangeInfo>& ranges,
                            const std::string& buffer,
                            const re2::RE2& regex,
                            std::vector<std::string>& captured_ids) {
  std::vector<bool> child_matched(ranges.size(), false);

  for (const ElementTextRangeInfo& range : ranges) {
    bool matched = false;
    if (!child_matched[range.frame_id] && !range.buffer_was_truncated) {
      std::string_view element_text = std::string_view(buffer).substr(
          range.text_start, range.text_end - range.text_start);
      if (std::optional<std::string> id =
              MaybeExtractConcatenatedGroups(element_text, regex)) {
        captured_ids.push_back(std::move(*id));
        matched = true;
      }
    }

    if ((matched || child_matched[range.frame_id]) && range.parent_frame_id) {
      child_matched[*range.parent_frame_id] = true;
    }
  }
}

}  // namespace

std::optional<std::string> MaybeValidateId(
    std::string id,
    base::optional_ref<const std::string> validation_pattern) {
  if (!validation_pattern) {
    return std::move(id);
  }

  const re2::RE2 regex(*validation_pattern, re2::RE2::Quiet);
  if (!regex.ok()) {
    return std::nullopt;
  }

  return re2::RE2::FullMatch(id, regex) ? std::optional(std::move(id))
                                        : std::nullopt;
}

std::optional<std::string> MaybeResolveIdSelector(
    const blink::WebDocument& web_document,
    const std::vector<mojom::OrderSelectorInfoPtr>& mojom_selectors,
    base::optional_ref<const std::string> domain_validation_pattern) {
  for (OrderSelectorMatchInfo& match :
       CollectSelectorMatches(web_document, mojom_selectors)) {
    // The selector's own `validation_pattern` takes precedence over the
    // domain-level validation pattern when validating the extracted ID.
    if (std::optional<std::string> id = MaybeValidateId(
            std::move(match.value), match.validation_pattern
                                        ? base::optional_ref<const std::string>(
                                              match.validation_pattern)
                                        : domain_validation_pattern)) {
      return id;
    }
  }
  return std::nullopt;
}

std::optional<std::string> MaybeFindCapturedIdInTextNodes(
    base::span<const blink::WebNode> text_nodes,
    const re2::RE2& regex,
    base::optional_ref<const std::string> validation_pattern) {
  for (const blink::WebNode& text_node : text_nodes) {
    std::optional<std::string> id =
        MaybeExtractConcatenatedGroups(text_node.NodeValue().Utf8(), regex);
    if (!id) {
      continue;
    }
    if (std::optional<std::string> validated_id =
            MaybeValidateId(std::move(*id), validation_pattern)) {
      return validated_id;
    }
  }
  return std::nullopt;
}

std::optional<std::string> MaybeFindCapturedIdUsingRegexes(
    const blink::WebDocument& web_document,
    base::span<const blink::WebNode> text_nodes,
    base::span<const std::string> id_regexes,
    size_t max_combined_text_length_to_search,
    size_t number_of_descendant_levels_to_search,
    base::optional_ref<const std::string> validation_pattern) {
  // Walked once, regardless of how many `id_regexes` there are, instead of
  // once per regex.
  std::string buffer;
  std::vector<ElementTextRangeInfo> ranges;
  const blink::WebElement document_element = web_document.DocumentElement();
  if (!document_element.IsNull()) {
    ranges = CollectElementTextRanges(
        document_element, max_combined_text_length_to_search,
        number_of_descendant_levels_to_search, buffer);
  }

  for (const std::string& id_regex : id_regexes) {
    const re2::RE2 regex(id_regex, re2::RE2::Quiet);
    if (!regex.ok()) {
      continue;
    }

    if (std::optional<std::string> id = MaybeFindCapturedIdInTextNodes(
            text_nodes, regex, validation_pattern)) {
      return id;
    }

    std::vector<std::string> captured_ids;
    MatchElementTextRanges(ranges, buffer, regex, captured_ids);

    // Validate each captured ID and return the first one that passes.
    for (std::string& captured_id : captured_ids) {
      if (std::optional<std::string> validated_id =
              MaybeValidateId(std::move(captured_id), validation_pattern)) {
        return validated_id;
      }
    }
  }
  return std::nullopt;
}

std::optional<std::string> MaybeExtractOrderId(
    const blink::WebDocument& web_document,
    base::span<const blink::WebNode> text_nodes,
    const mojom::OrderPatternsInfo& mojom_order_patterns) {
  // Try to resolve the order ID using the provided ID selectors first.
  if (std::optional<std::string> resolved_id = MaybeResolveIdSelector(
          web_document, mojom_order_patterns.id_selectors,
          mojom_order_patterns.id_validation_pattern)) {
    return resolved_id;
  }

  // If no ID is resolved using selectors, fall back to matching regexes
  // against individual text nodes, then combined multi-element text.
  return MaybeFindCapturedIdUsingRegexes(
      web_document, text_nodes, mojom_order_patterns.id_regexes,
      mojom_order_patterns.id_max_combined_text_length_to_search,
      mojom_order_patterns.id_number_of_descendant_levels_to_search,
      mojom_order_patterns.id_validation_pattern);
}

}  // namespace brave_ads
