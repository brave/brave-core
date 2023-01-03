/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/targeting/models/contextual/text_classification/text_classification_model.h"

#include <string>

#include "base/check.h"
#include "bat/ads/internal/ads/serving/targeting/models/contextual/text_classification/text_classification_alias.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/deprecated/client/client_state_manager.h"
#include "brave/components/l10n/common/locale_util.h"

namespace ads::targeting::model {

namespace {

SegmentProbabilityMap GetSegmentProbabilities(
    const TextClassificationProbabilityList&
        text_classification_probabilities) {
  SegmentProbabilityMap segment_probabilities;

  for (const auto& probabilities : text_classification_probabilities) {
    for (const auto& probability : probabilities) {
      const std::string segment = probability.first;
      DCHECK(!segment.empty());

      const double page_score = probability.second;

      const auto iter = segment_probabilities.find(segment);
      if (iter == segment_probabilities.cend()) {
        SegmentProbabilityPair segment_probability = {segment, page_score};
        segment_probabilities.insert(segment_probability);
      } else {
        iter->second += page_score;
      }
    }
  }

  return segment_probabilities;
}

SegmentProbabilityList ToSortedSegmentProbabilityList(
    const SegmentProbabilityMap& segment_probabilities) {
  const int count = segment_probabilities.size();
  SegmentProbabilityList list(count);

  std::partial_sort_copy(
      segment_probabilities.cbegin(), segment_probabilities.cend(),
      list.begin(), list.end(),
      [](const SegmentProbabilityPair& lhs, const SegmentProbabilityPair& rhs) {
        return lhs.second > rhs.second;
      });

  return list;
}

SegmentList ToSegmentList(const SegmentProbabilityList& segment_probabilities) {
  SegmentList segments;

  for (const auto& segment_probability : segment_probabilities) {
    const std::string segment = segment_probability.first;
    DCHECK(!segment.empty());

    segments.push_back(segment);
  }

  return segments;
}

}  // namespace

SegmentList TextClassification::GetSegments() const {
  const TextClassificationProbabilityList& probabilities =
      ClientStateManager::GetInstance()
          ->GetTextClassificationProbabilitiesHistory();

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

}  // namespace ads::targeting::model
