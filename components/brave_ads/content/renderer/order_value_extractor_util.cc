/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/renderer/order_value_extractor_util.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <set>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "third_party/blink/public/web/web_node.h"
#include "third_party/re2/src/re2/re2.h"

// RE2 has no backtracking at all, so matching is linear-time regardless of
// pattern shape. Chromium's own BNPL amount extraction (components/autofill)
// also matches remote, component-delivered patterns against page text, using
// V8's backtracking `ScriptRegexp`, but caps the damage via
// `RegExp::NewWithBacktrackLimit()` rather than avoiding it structurally, and
// its own patterns stay small and scoped to allow-listed partner domains.
// Ours are larger and run against arbitrary merchant pages, so RE2's
// structural guarantee matters more here than a step-count cap would.

namespace brave_ads {

re2::RE2 JoinRegexes(base::span<const std::string> regexes) {
  if (regexes.empty()) {
    // An empty alternation joins to the empty pattern, which matches
    // everywhere. Use a character class excluding every valid code point
    // instead, so an empty list of regexes matches nothing.
    return re2::RE2("[^\\x00-\\x{10FFFF}]", re2::RE2::Quiet);
  }

  std::vector<std::string> non_capturing_regexes;
  non_capturing_regexes.reserve(regexes.size());
  std::ranges::transform(regexes, std::back_inserter(non_capturing_regexes),
                         [](const std::string& regex) {
                           return base::StrCat({"(?:", regex, ")"});
                         });
  return re2::RE2(base::JoinString(non_capturing_regexes, "|"),
                  re2::RE2::Quiet);
}

std::optional<std::string> MatchCurrency(
    const std::string& text,
    const base::flat_map<std::string, std::string>& currency_regexes) {
  for (const auto& [currency, pattern] : currency_regexes) {
    const re2::RE2 regex(pattern, re2::RE2::Quiet);
    if (regex.ok() && re2::RE2::PartialMatch(text, regex)) {
      return currency;
    }
  }
  return std::nullopt;
}

// Splits `text_nodes` into those matching `currency_regex` and those
// matching `keyword_regex`; a node may land in both when a site combines
// the label and price in one text node.
std::pair<std::vector<blink::WebNode>, std::vector<blink::WebNode>>
CollectCurrencyAndKeywordNodes(base::span<const blink::WebNode> text_nodes,
                               const re2::RE2& currency_regex,
                               const re2::RE2& keyword_regex) {
  std::vector<blink::WebNode> currency_web_nodes;
  std::vector<blink::WebNode> keyword_web_nodes;
  for (const blink::WebNode& web_node : text_nodes) {
    const std::string text = web_node.NodeValue().Utf8();
    if (re2::RE2::PartialMatch(text, currency_regex)) {
      currency_web_nodes.push_back(web_node);
    }
    if (re2::RE2::PartialMatch(text, keyword_regex)) {
      keyword_web_nodes.push_back(web_node);
    }
  }
  return {std::move(currency_web_nodes), std::move(keyword_web_nodes)};
}

// Every ancestor of every node in `web_nodes`, so a currency node's own
// ancestor chain can be checked for overlap with a keyword node's.
std::set<blink::WebNode> CollectAncestors(
    base::span<const blink::WebNode> web_nodes) {
  std::set<blink::WebNode> ancestor_web_nodes;
  for (const blink::WebNode& web_node : web_nodes) {
    blink::WebNode ancestor_web_node = web_node.ParentNode();
    while (ancestor_web_node &&
           ancestor_web_nodes.insert(ancestor_web_node).second) {
      ancestor_web_node = ancestor_web_node.ParentNode();
    }
  }
  return ancestor_web_nodes;
}

// Climbs each currency node's ancestor chain up to
// `number_of_ancestor_levels_to_search` levels, looking for an ancestor
// also reached by a keyword node. An ancestor shared by more than one
// currency node is ambiguous (which price does the label refer to?) and
// is excluded from every currency node's search from then on.
std::optional<blink::WebNode> FindUnambiguousCurrencyMatch(
    base::span<const blink::WebNode> currency_web_nodes,
    const std::set<blink::WebNode>& keyword_ancestor_web_nodes,
    uint32_t number_of_ancestor_levels_to_search) {
  std::map<blink::WebNode, size_t> currency_web_node_count_by_ancestor;
  std::vector<std::pair<blink::WebNode, blink::WebNode>>
      currency_web_node_to_ancestor;
  currency_web_node_to_ancestor.reserve(currency_web_nodes.size());
  for (const blink::WebNode& currency_web_node : currency_web_nodes) {
    blink::WebNode ancestor_web_node = currency_web_node.ParentNode();
    currency_web_node_to_ancestor.emplace_back(currency_web_node,
                                               ancestor_web_node);
    while (ancestor_web_node) {
      ++currency_web_node_count_by_ancestor[ancestor_web_node];
      ancestor_web_node = ancestor_web_node.ParentNode();
    }
  }

  for (uint32_t level = 0; level < number_of_ancestor_levels_to_search;
       ++level) {
    for (auto& [currency_web_node, ancestor_web_node] :
         currency_web_node_to_ancestor) {
      if (!ancestor_web_node) {
        continue;
      }
      if (currency_web_node_count_by_ancestor[ancestor_web_node] > 1) {
        ancestor_web_node.Reset();
        continue;
      }
      if (keyword_ancestor_web_nodes.contains(ancestor_web_node)) {
        return currency_web_node;
      }
      ancestor_web_node = ancestor_web_node.ParentNode();
    }
  }
  return std::nullopt;
}

std::optional<OrderValueMatchInfo> FindLabeledTextNodeValue(
    base::span<const blink::WebNode> text_nodes,
    const base::flat_map<std::string, std::string>& currency_regexes,
    const re2::RE2& keyword_regex,
    uint32_t number_of_ancestor_levels_to_search) {
  std::vector<std::string> currency_patterns;
  currency_patterns.reserve(currency_regexes.size());
  for (const auto& [currency, pattern] : currency_regexes) {
    currency_patterns.push_back(pattern);
  }
  const re2::RE2 currency_regex = JoinRegexes(currency_patterns);
  if (!currency_regex.ok()) {
    return std::nullopt;
  }

  auto [currency_web_nodes, keyword_web_nodes] =
      CollectCurrencyAndKeywordNodes(text_nodes, currency_regex, keyword_regex);
  if (currency_web_nodes.empty() || keyword_web_nodes.empty()) {
    return std::nullopt;
  }

  const std::set<blink::WebNode> keyword_ancestor_web_nodes =
      CollectAncestors(keyword_web_nodes);

  std::optional<blink::WebNode> currency_web_node =
      FindUnambiguousCurrencyMatch(currency_web_nodes,
                                   keyword_ancestor_web_nodes,
                                   number_of_ancestor_levels_to_search);
  if (!currency_web_node) {
    return std::nullopt;
  }

  std::string value = currency_web_node->NodeValue().Utf8();
  return OrderValueMatchInfo{
      .value = value,
      .currency = MatchCurrency(value, currency_regexes),
  };
}

std::optional<std::string> ExtractBareNumber(const std::string& candidate) {
  // The thousands-group quantifier must be `+`, not `*`; RE2 matches
  // leftmost-first, so `*` would truncate an ungrouped number to 1-3 digits.
  static constexpr std::string_view kBareNumberPattern =
      R"RE2(((?:\d{1,3}(?:[.,\p{Zs}]\d{3})+|\d+)(?:[.,]\d{2})?))RE2";
  const re2::RE2 regex(kBareNumberPattern, re2::RE2::Quiet);
  if (!regex.ok()) {
    return std::nullopt;
  }

  // The last number, not the first, since an unrelated reference/customer
  // number is more likely to precede the real price in the same text than
  // follow it.
  std::string_view remaining = candidate;
  std::string number;
  std::string match;
  bool matched = false;
  while (re2::RE2::FindAndConsume(&remaining, regex, &match)) {
    number = match;
    matched = true;
  }
  return matched ? std::make_optional(std::move(number)) : std::nullopt;
}

}  // namespace brave_ads
