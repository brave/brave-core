/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>
#include <algorithm>
#include <sstream>

#include "bat/ads/internal/classification/page_classifier/page_classifier_util.h"
#include "bat/ads/internal/classification/purchase_intent_classifier/keywords.h"

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"

namespace ads {
namespace classification {

const uint16_t kPurchaseIntentWordCountLimit = 1000;

Keywords::Keywords() = default;
Keywords::~Keywords() = default;

std::string SanitizeInput(
    const std::string& content) {
  if (content.empty()) {
    return "";
  }

  std::string stripped_content = content;

  const std::string escaped_characters =
      RE2::QuoteMeta("!\"#$%&'()*+,-./:<=>?@\\[]^_`{|}~");

  const std::string pattern = base::StringPrintf("[[:cntrl:]]|"
      "\\\\(t|n|v|f|r)|[\\t\\n\\v\\f\\r]|\\\\x[[:xdigit:]][[:xdigit:]]|"
          "[%s]", escaped_characters.c_str());

  RE2::GlobalReplace(&stripped_content, pattern, " ");

  return base::CollapseWhitespaceASCII(stripped_content, true);
}

PurchaseIntentSegmentList Keywords::GetSegments(
    const std::string& search_query) {
  PurchaseIntentSegmentList segment_list;
  auto search_query_keyword_set = TransformIntoSetOfWords(search_query);

  for (const auto& keyword : _segment_keywords) {
    auto list_keyword_set = TransformIntoSetOfWords(keyword.keywords);

    // Intended behaviour relies on early return from list traversal and
    // implicitely on the ordering of |_segment_keywords| to ensure
    // specific segments are matched over general segments, e.g. "audi a6"
    // segments should be returned over "audi" segments if possible.
    if (Keywords::IsSubset(search_query_keyword_set, list_keyword_set)) {
      segment_list = keyword.segments;
      return segment_list;
    }
  }

  return segment_list;
}

uint16_t Keywords::GetFunnelWeight(
    const std::string& search_query) {
  auto search_query_keyword_set = TransformIntoSetOfWords(search_query);

  uint16_t max_weight = _default_signal_weight;
  for (const auto& keyword : _automotive_funnel_keywords) {
    auto list_keyword_set = TransformIntoSetOfWords(keyword.keywords);

    if (Keywords::IsSubset(search_query_keyword_set, list_keyword_set) &&
        keyword.weight > max_weight) {
      max_weight = keyword.weight;
    }
  }

  return max_weight;
}

// TODO(Moritz Haller): Rename to make explicit that method checks set_a is
// subset of set_b but not vice versa
// TODO(https://github.com/brave/brave-browser/issues/8495): Implement Brave
// Ads Purchase Intent keyword matching with std::sets
bool Keywords::IsSubset(
    std::vector<std::string> keyword_set_a,
    std::vector<std::string> keyword_set_b) {
  std::sort(keyword_set_a.begin(), keyword_set_a.end());
  std::sort(keyword_set_b.begin(), keyword_set_b.end());

  return std::includes(keyword_set_a.begin(), keyword_set_a.end(),
      keyword_set_b.begin(), keyword_set_b.end());
}

// TODO(https://github.com/brave/brave-browser/issues/8495): Implement Brave
// Ads Purchase Intent keyword matching with std::sets
std::vector<std::string> Keywords::TransformIntoSetOfWords(
    const std::string& text) {
  std::string lowercase_text = SanitizeInput(text);
  std::transform(lowercase_text.begin(), lowercase_text.end(),
      lowercase_text.begin(), ::tolower);
  std::stringstream sstream(lowercase_text);
  std::vector<std::string> set_of_words;
  std::string word;
  uint16_t word_count = 0;

  while (sstream >> word && word_count < kPurchaseIntentWordCountLimit) {
    set_of_words.push_back(word);
    word_count++;
  }

  return set_of_words;
}

}  // namespace classification
}  // namespace ads
