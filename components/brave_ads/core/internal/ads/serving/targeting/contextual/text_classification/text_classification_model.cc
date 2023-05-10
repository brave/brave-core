/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/targeting/contextual/text_classification/text_classification_model.h"

#include <string>

#include "base/check.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/contextual/text_classification/text_classification_alias.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/l10n/common/locale_util.h"

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

  base::ranges::partial_sort_copy(
      segment_probabilities, list,
      [](const SegmentProbabilityPair& lhs, const SegmentProbabilityPair& rhs) {
        return lhs.second > rhs.second;
      });

  return list;
}

SegmentList ToSegmentList(const SegmentProbabilityList& segment_probabilities) {
  SegmentList segments;

  for (const auto& [segment, probability] : segment_probabilities) {
    CHECK(!segment.empty());

    segments.push_back(segment);
  }

  return segments;
}

}  // namespace

SegmentList TextClassificationModel::GetSegments() const {
  const TextClassificationProbabilityList& probabilities =
      ClientStateManager::GetInstance()
          .GetTextClassificationProbabilitiesHistory();

  if (probabilities.empty()) {
    BLOG(1, "No text classification probabilities found for "
                << brave_l10n::GetDefaultLocaleString() << " locale");

    return {};
  }

  const SegmentProbabilityMap segment_probabilities =
      GetSegmentProbabilities(probabilities);

  const SegmentProbabilityList sorted_segment_probabilities =
      ToSortedSegmentProbabilityList(segment_probabilities);

  return ToSegmentList(sorted_segment_probabilities);
}

}  // namespace brave_ads
