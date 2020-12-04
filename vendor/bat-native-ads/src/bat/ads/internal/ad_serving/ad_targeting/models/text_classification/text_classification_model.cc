/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/ad_targeting/models/text_classification/text_classification_model.h"

#include <algorithm>
#include <deque>
#include <map>
#include <string>
#include <vector>

#include "brave/components/l10n/browser/locale_helper.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_util.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/client/preferences/filtered_category_info.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace ad_targeting {
namespace model {

namespace {
const int kTopSegmentCount = 3;
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

///////////////////////////////////////////////////////////////////////////////

SegmentProbabilitiesMap TextClassification::GetSegmentProbabilities(
    const TextClassificationProbabilitiesList&
        text_classifications_probabilities) const {
  SegmentProbabilitiesMap segment_probabilities;

  for (const auto& probabilities : text_classifications_probabilities) {
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

bool TextClassification::ShouldFilterSegment(
    const std::string& segment) const {
  // If passed in segment has a sub segment and the current filter does not,
  // check if it's a child of the filter. Conversely, if the passed in segment
  // has no sub segment but the current filter does, it can't be a match at all
  // so move on to the next filter. Otherwise, perform an exact match to
  // determine whether or not to filter the segment

  const std::vector<std::string> segment_classifications =
      SplitSegment(segment);

  const FilteredCategoryList filtered_segments =
      Client::Get()->get_filtered_categories();

  for (const auto& filtered_segment : filtered_segments) {
    const std::vector<std::string> filtered_segment_classifications =
        SplitSegment(filtered_segment.name);

    if (segment_classifications.size() > 1 &&
        filtered_segment_classifications.size() == 1) {
      if (segment_classifications.front() ==
          filtered_segment_classifications.front()) {
        return true;
      }
    } else if (segment_classifications.size() == 1 &&
        filtered_segment_classifications.size() > 1) {
      continue;
    } else if (filtered_segment.name == segment) {
      return true;
    }
  }

  return false;
}

SegmentProbabilitiesList TextClassification::GetTopSegmentProbabilities(
    const SegmentProbabilitiesMap& segment_probabilities,
    const int count) const {
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

SegmentList TextClassification::ToSegmentList(
    const SegmentProbabilitiesList segment_probabilities) const {
  SegmentList segments;

  for (const auto& segment_probability : segment_probabilities) {
    const std::string segment = segment_probability.first;
    segments.push_back(segment);
  }

  return segments;
}

}  // namespace model
}  // namespace ad_targeting
}  // namespace ads
