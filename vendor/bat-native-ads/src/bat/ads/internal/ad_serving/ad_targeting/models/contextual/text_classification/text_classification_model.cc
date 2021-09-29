/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/ad_targeting/models/contextual/text_classification/text_classification_model.h"

#include <string>

#include "bat/ads/internal/ad_targeting/data_types/contextual/text_classification/text_classification_aliases.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/logging.h"
#include "brave/components/l10n/browser/locale_helper.h"

namespace ads {
namespace ad_targeting {
namespace model {

namespace {

SegmentProbabilitiesMap GetSegmentProbabilities(
    const TextClassificationProbabilitiesList&
        text_classification_probabilities) {
  SegmentProbabilitiesMap segment_probabilities;

  for (const auto& probabilities : text_classification_probabilities) {
    for (const auto& probability : probabilities) {
      const std::string segment = probability.first;

      const double page_score = probability.second;

      const auto iter = segment_probabilities.find(segment);
      if (iter == segment_probabilities.end()) {
        SegmentProbabilityPair segment_probability = {segment, page_score};
        segment_probabilities.insert(segment_probability);
      } else {
        iter->second += page_score;
      }
    }
  }

  return segment_probabilities;
}

SegmentProbabilitiesList ToSortedSegmentProbabilitiesList(
    const SegmentProbabilitiesMap& segment_probabilities) {
  const int count = segment_probabilities.size();
  SegmentProbabilitiesList list(count);

  std::partial_sort_copy(
      segment_probabilities.begin(), segment_probabilities.end(), list.begin(),
      list.end(),
      [](const SegmentProbabilityPair& lhs, const SegmentProbabilityPair& rhs) {
        return lhs.second > rhs.second;
      });

  return list;
}

SegmentList ToSegmentList(
    const SegmentProbabilitiesList& segment_probabilities) {
  SegmentList segments;

  for (const auto& segment_probability : segment_probabilities) {
    const std::string segment = segment_probability.first;
    segments.push_back(segment);
  }

  return segments;
}

}  // namespace

TextClassification::TextClassification() = default;

TextClassification::~TextClassification() = default;

SegmentList TextClassification::GetSegments() const {
  const TextClassificationProbabilitiesList probabilities =
      Client::Get()->GetTextClassificationProbabilitiesHistory();

  if (probabilities.empty()) {
    const std::string locale =
        brave_l10n::LocaleHelper::GetInstance()->GetLocale();
    BLOG(1, "No text classification probabilities found for " << locale
                                                              << " locale");

    return {};
  }

  const SegmentProbabilitiesMap segment_probabilities =
      GetSegmentProbabilities(probabilities);

  const SegmentProbabilitiesList sorted_segment_probabilities =
      ToSortedSegmentProbabilitiesList(segment_probabilities);

  return ToSegmentList(sorted_segment_probabilities);
}

}  // namespace model
}  // namespace ad_targeting
}  // namespace ads
