/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>
#include <algorithm>
#include <sstream>

#include "url/gurl.h"
#include "third_party/re2/src/re2/re2.h"
#include "bat/ads/internal/purchase_intent/keywords.h"

namespace ads {

Keywords::Keywords() = default;
Keywords::~Keywords() = default;

PurchaseIntentSegmentList Keywords::GetSegments(
    const std::string& search_query) {
  PurchaseIntentSegmentList segment_list;
  auto search_query_keyword_set = TransformIntoSetOfWords(search_query);

  for (const auto& keyword : _automotive_segment_keywords) {
    auto list_keyword_set = TransformIntoSetOfWords(keyword.keywords);

    // Intended behaviour relies on early return from list traversal and
    // implicitely on the ordering of `_automotive_segment_keywords` to ensure
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
  std::string data = text;
  // Remove every character that is not a word/whitespace/underscore character
  RE2::GlobalReplace(&data, "[^\\w\\s]|_", "");
  // Strip subsequent white space characters
  RE2::GlobalReplace(&data, "\\s+", " ");

  std::for_each(data.begin(), data.end(), [](char & c) {
    c = base::ToLowerASCII(c);
  });

  std::stringstream sstream(data);
  std::vector<std::string> set_of_words;
  std::string word;
  uint16_t word_count = 0;

  while (sstream >> word && word_count < _word_count_limit) {
    set_of_words.push_back(word);
    word_count++;
  }

  return set_of_words;
}

}  // namespace ads
