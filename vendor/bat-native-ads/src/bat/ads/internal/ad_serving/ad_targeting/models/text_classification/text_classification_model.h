/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_SERVING_AD_TARGETING_MODELS_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_MODEL_H_  // NOLINT
#define BAT_ADS_INTERNAL_AD_SERVING_AD_TARGETING_MODELS_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_MODEL_H_  // NOLINT

#include <deque>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "bat/ads/internal/ad_serving/ad_targeting/models/model.h"

namespace ads {
namespace ad_targeting {
namespace model {

using TextClassificationProbabilitiesMap = std::map<std::string, double>;
using TextClassificationProbabilitiesList =
    std::deque<TextClassificationProbabilitiesMap>;

using SegmentProbabilityPair = std::pair<std::string, double>;
using SegmentProbabilitiesList = std::vector<SegmentProbabilityPair>;
using SegmentProbabilitiesMap = std::map<std::string, double>;

const char kUntargeted[] = "untargeted";

class TextClassification : public Model {
 public:
  TextClassification();

  ~TextClassification() override;

  SegmentList GetSegments() const override;

 private:
  SegmentProbabilitiesMap GetSegmentProbabilities(
      const TextClassificationProbabilitiesList&
          text_classification_probabilities) const;

  bool ShouldFilterSegment(
      const std::string& segment) const;

  SegmentProbabilitiesList GetTopSegmentProbabilities(
      const SegmentProbabilitiesMap& segment_probabilities,
      const int count) const;

  SegmentList ToSegmentList(
      const SegmentProbabilitiesList segment_probabilities) const;
};

}  // namespace model
}  // namespace ad_targeting
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_SERVING_AD_TARGETING_MODELS_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_MODEL_H_  // NOLINT
