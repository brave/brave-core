/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/model/purchase_intent_model_segment_predictor.h"

#include <cstddef>

#include "base/containers/adapters.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_feature.h"

namespace brave_ads {

namespace {
constexpr size_t kMaximumSegments = 3;
}  // namespace

SegmentList PredictPurchaseIntentSegments(
    const std::multimap<int, std::string>& segment_scores) {
  if (segment_scores.empty()) {
    return {};
  }

  const int threshold = kPurchaseIntentThreshold.Get();

  SegmentList segments;
  segments.reserve(kMaximumSegments);

  for (const auto& [score, segment] : base::Reversed(segment_scores)) {
    if (score < threshold) {
      continue;
    }

    segments.push_back(segment);

    if (segments.size() == segment_scores.size() ||
        segments.size() >= kMaximumSegments) {
      break;
    }
  }

  return segments;
}

}  // namespace brave_ads
