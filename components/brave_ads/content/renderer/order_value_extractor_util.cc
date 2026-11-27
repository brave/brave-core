/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/renderer/order_value_extractor_util.h"

#include <map>
#include <memory>
#include <set>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom.h"
#include "brave/components/brave_ads/content/renderer/order_extractor_selector_util.h"
#include "brave/components/brave_ads/content/renderer/order_selector_match_info.h"
#include "brave/components/brave_ads/content/renderer/order_value_extractor_util_constants.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_node.h"
#include "third_party/re2/src/re2/re2.h"

namespace brave_ads {

namespace {

// Returns true if `text` contains only whitespace and the `bare_number`.
bool IsOnlyWhitespaceAndBareNumber(std::string_view text,
                                   std::string_view bare_number) {
  const size_t position = text.find(bare_number);
  if (position == std::string_view::npos) {
    return false;
  }

  std::string remainder(text.substr(0, position));
  remainder.append(text.substr(position + bare_number.size()));

  std::string trimmed_remainder;
  base::TrimWhitespaceASCII(remainder, base::TRIM_ALL, &trimmed_remainder);
  return trimmed_remainder.empty();
}

// Returns a pair of vectors containing the text nodes that match the currency
// and keyword regexes, respectively.
std::pair<std::vector<blink::WebNode>, std::vector<blink::WebNode>>
CollectCurrencyAndKeywordNodes(base::span<const blink::WebNode> text_nodes,
                               const re2::RE2& currency_regex,
                               const re2::RE2& keyword_regex) {
  std::vector<blink::WebNode> currency_web_nodes;
  std::vector<blink::WebNode> keyword_web_nodes;
  for (const blink::WebNode& text_node : text_nodes) {
    const std::string text = text_node.NodeValue().Utf8();
    if (re2::RE2::PartialMatch(text, currency_regex)) {
      currency_web_nodes.push_back(text_node);
    }
    if (re2::RE2::PartialMatch(text, keyword_regex)) {
      keyword_web_nodes.push_back(text_node);
    }
  }
  return {std::move(currency_web_nodes), std::move(keyword_web_nodes)};
}

// Returns a set of all ancestor nodes for the given web nodes.
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

// Finds a currency web node whose ancestor does not contain any keyword web
// nodes, up to the specified number of ancestor levels to search.
std::optional<blink::WebNode> MaybeFindCurrencyMatch(
    base::span<const blink::WebNode> currency_web_nodes,
    const std::set<blink::WebNode>& keyword_ancestor_web_nodes,
    size_t number_of_ancestor_levels_to_search) {
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

  for (size_t level = 0; level < number_of_ancestor_levels_to_search; ++level) {
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

}  // namespace

std::optional<OrderValueMatchInfo> MaybeResolveValueSelector(
    const blink::WebDocument& web_document,
    const std::vector<mojom::OrderSelectorInfoPtr>& mojom_selectors,
    const base::flat_map<std::string, std::string>& currency_regexes) {
  std::optional<OrderValueMatchInfo> currencyless_match;
  for (const OrderSelectorMatchInfo& match :
       CollectSelectorMatches(web_document, mojom_selectors)) {
    std::optional<std::string> value = MaybeExtractBareNumber(match.value);
    if (!value) {
      continue;
    }

    std::optional<std::string> currency =
        MaybeMatchCurrency(match.value, currency_regexes);
    if (IsOnlyWhitespaceAndBareNumber(match.value, *value)) {
      // If the text contains only whitespace and the bare number, any matched
      // currency is not considered reliable and is ignored.
      currency = std::nullopt;
    } else if (!currency) {
      // If the text contains a bare number but no currency, we cannot trust it
      // to indicate a specific currency, so we skip this match.
      continue;
    }

    if (currency) {
      return OrderValueMatchInfo{.value = std::move(*value),
                                 .currency = std::move(currency)};
    }

    if (!currencyless_match) {
      currencyless_match = OrderValueMatchInfo{.value = std::move(*value),
                                               .currency = std::nullopt};
    }
  }
  return currencyless_match;
}

std::unique_ptr<re2::RE2> JoinRegexes(base::span<const std::string> regexes) {
  std::vector<std::string> non_capturing_regexes;
  non_capturing_regexes.reserve(regexes.size());
  for (const std::string& regex : regexes) {
    std::string non_capturing_regex = base::StrCat({"(?:", regex, ")"});
    if (re2::RE2(non_capturing_regex, re2::RE2::Quiet).ok()) {
      non_capturing_regexes.push_back(std::move(non_capturing_regex));
    }
  }

  if (non_capturing_regexes.empty()) {
    return nullptr;
  }
  return std::make_unique<re2::RE2>(
      base::JoinString(non_capturing_regexes, "|"), re2::RE2::Quiet);
}

std::optional<std::string> MaybeMatchCurrency(
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

std::optional<OrderValueMatchInfo> MaybeFindLabeledTextNodeValue(
    base::span<const blink::WebNode> text_nodes,
    const base::flat_map<std::string, std::string>& currency_regexes,
    const re2::RE2& keyword_regex,
    size_t number_of_ancestor_levels_to_search) {
  std::vector<std::string> currency_patterns;
  currency_patterns.reserve(currency_regexes.size());
  for (const auto& [currency, pattern] : currency_regexes) {
    currency_patterns.push_back(pattern);
  }
  std::unique_ptr<re2::RE2> currency_regex = JoinRegexes(currency_patterns);
  if (!currency_regex) {
    return std::nullopt;
  }

  auto [currency_web_nodes, keyword_web_nodes] = CollectCurrencyAndKeywordNodes(
      text_nodes, *currency_regex, keyword_regex);
  if (currency_web_nodes.empty() || keyword_web_nodes.empty()) {
    return std::nullopt;
  }

  const std::set<blink::WebNode> keyword_ancestor_web_nodes =
      CollectAncestors(keyword_web_nodes);

  std::optional<blink::WebNode> currency_web_node =
      MaybeFindCurrencyMatch(currency_web_nodes, keyword_ancestor_web_nodes,
                             number_of_ancestor_levels_to_search);
  if (!currency_web_node) {
    return std::nullopt;
  }

  // Extract the text content of the currency node for further processing.
  std::string value = currency_web_node->NodeValue().Utf8();
  std::optional<std::string> currency =
      MaybeMatchCurrency(value, currency_regexes);
  if (std::optional<std::string> bare_number = MaybeExtractBareNumber(value);
      bare_number && IsOnlyWhitespaceAndBareNumber(value, *bare_number)) {
    // If the value contains only whitespace and a bare number, it cannot be
    // reliably associated with a specific currency, so clear the currency.
    currency = std::nullopt;
  }
  return OrderValueMatchInfo{.value = std::move(value),
                             .currency = std::move(currency)};
}

std::optional<std::string> MaybeExtractBareNumber(const std::string& text) {
  const re2::RE2 regex(kBareNumberPattern, re2::RE2::Quiet);
  if (!regex.ok()) {
    return std::nullopt;
  }

  std::string_view remaining = text;
  std::string number;
  std::string match;
  bool matched = false;
  while (re2::RE2::FindAndConsume(&remaining, regex, &match)) {
    number = match;
    matched = true;
  }
  return matched ? std::make_optional(std::move(number)) : std::nullopt;
}

std::optional<OrderValueMatchInfo> MaybeExtractOrderValue(
    const blink::WebDocument& web_document,
    base::span<const blink::WebNode> text_nodes,
    const mojom::OrderPatternsInfo& mojom_order_patterns) {
  if (std::optional<OrderValueMatchInfo> selector_match =
          MaybeResolveValueSelector(
              web_document, mojom_order_patterns.value_selectors,
              mojom_order_patterns.value_currency_regexes)) {
    return selector_match;
  }

  std::unique_ptr<re2::RE2> keyword_regex =
      JoinRegexes(mojom_order_patterns.value_keyword_regexes);
  if (!keyword_regex) {
    return std::nullopt;
  }

  std::optional<OrderValueMatchInfo> keyword_match =
      MaybeFindLabeledTextNodeValue(
          text_nodes, mojom_order_patterns.value_currency_regexes,
          *keyword_regex,
          mojom_order_patterns.number_of_ancestor_levels_to_search);
  if (!keyword_match) {
    return std::nullopt;
  }

  std::optional<std::string> bare_number =
      MaybeExtractBareNumber(keyword_match->value);
  if (!bare_number) {
    return std::nullopt;
  }

  return OrderValueMatchInfo{.value = std::move(*bare_number),
                             .currency = std::move(keyword_match->currency)};
}

}  // namespace brave_ads
