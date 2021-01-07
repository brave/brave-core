/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/ad_targeting/models/contextual/text_classification/text_classification_model.h"

#include <string>

#include "brave/components/l10n/browser/locale_helper.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_segment_util.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_values.h"
#include "bat/ads/internal/ad_targeting/data_types/contextual/text_classification/text_classification_aliases.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace ad_targeting {
namespace model {

namespace {

const int kTopSegmentCount = 3;

SegmentProbabilitiesMap GetSegmentProbabilities(
    const TextClassificationProbabilitiesList&
        text_classification_probabilities) {
  SegmentProbabilitiesMap segment_probabilities;

  for (const auto& probabilities : text_classification_probabilities) {
    for (const auto& probability : probabilities) {
      const std::string segment = probability.first;
      if (ShouldFilterSegment(segment)) {
        continue;
      }

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

SegmentProbabilitiesList GetTopSegmentProbabilities(
    const SegmentProbabilitiesMap& segment_probabilities,
    const int count) {
  SegmentProbabilitiesList top_segment_probabilities(count);

  std::partial_sort_copy(segment_probabilities.begin(),
      segment_probabilities.end(), top_segment_probabilities.begin(),
          top_segment_probabilities.end(), [](
              const SegmentProbabilityPair& lhs,
                  const SegmentProbabilityPair& rhs) {
    return lhs.second > rhs.second;
  });

  return top_segment_probabilities;
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

    return {
      kUntargeted
    };
  }

  const SegmentProbabilitiesMap segment_probabilities =
      GetSegmentProbabilities(probabilities);

  const SegmentProbabilitiesList top_segment_probabilities =
      GetTopSegmentProbabilities(segment_probabilities, kTopSegmentCount);

  return ToSegmentList(top_segment_probabilities);
}

}  // namespace model
}  // namespace ad_targeting
}  // namespace ads
