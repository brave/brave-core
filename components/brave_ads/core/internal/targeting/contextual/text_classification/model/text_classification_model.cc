/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/model/text_classification_model.h"

#include <algorithm>
#include <iterator>

#include "base/check.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/common/locale/locale_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/model/text_classification_alias.h"

namespace brave_ads {

namespace {

SegmentProbabilityMap GetSegmentProbabilities(
    const TextClassificationProbabilityList&
        text_classification_probabilities) {
  SegmentProbabilityMap segment_probabilities;

  for (const auto& probabilities : text_classification_probabilities) {
    for (const auto& [segment, page_score] : probabilities) {
      CHECK(!segment.empty());

      const auto iter = segment_probabilities.find(segment);
      if (iter == segment_probabilities.cend()) {
        segment_probabilities.insert({segment, page_score});
      } else {
        iter->second += page_score;
      }
    }
  }

  return segment_probabilities;
}

SegmentProbabilityList ToSortedSegmentProbabilityList(
    const SegmentProbabilityMap& segment_probabilities) {
  SegmentProbabilityList list(segment_probabilities.size());
  list.reserve(segment_probabilities.size());

  base::ranges::partial_sort_copy(
      segment_probabilities, list,
      [](const auto& lhs, const auto& rhs) { return lhs.second > rhs.second; });

  return list;
}

SegmentList ToSegmentList(const SegmentProbabilityList& segment_probabilities) {
  SegmentList segments;
  segments.reserve(segment_probabilities.size());

  std::transform(segment_probabilities.cbegin(), segment_probabilities.cend(),
                 std::back_inserter(segments),
                 [](const auto& segment_probability) {
                   CHECK(!segment_probability.first.empty());
                   return segment_probability.first;
                 });

  return segments;
}

}  // namespace

SegmentList GetTextClassificationSegments() {
  const TextClassificationProbabilityList& probabilities =
      ClientStateManager::GetInstance()
          .GetTextClassificationProbabilitiesHistory();

  if (probabilities.empty()) {
    BLOG(1, "No text classification probabilities found for " << GetLocale()
                                                              << " locale");

    return {};
  }

  const SegmentProbabilityMap segment_probabilities =
      GetSegmentProbabilities(probabilities);

  const SegmentProbabilityList sorted_segment_probabilities =
      ToSortedSegmentProbabilityList(segment_probabilities);

  return ToSegmentList(sorted_segment_probabilities);
}

}  // namespace brave_ads
