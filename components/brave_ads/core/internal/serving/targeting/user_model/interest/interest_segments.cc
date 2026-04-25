/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/interest/interest_segments.h"

#include <utility>

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/model/text_classification_model.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_feature.h"

namespace brave_ads {

void BuildInterestSegments(BuildSegmentsCallback callback) {
  SegmentList segments;

  if (base::FeatureList::IsEnabled(kTextClassificationFeature)) {
    segments = GetTextClassificationSegments();
  }

  std::move(callback).Run(segments);
}

}  // namespace brave_ads
