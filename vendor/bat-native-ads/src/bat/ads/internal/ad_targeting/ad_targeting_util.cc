/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/ad_targeting_util.h"

#include "bat/ads/internal/ad_serving/ad_targeting/models/behavioral/bandits/epsilon_greedy_bandit_model.h"
#include "bat/ads/internal/ad_serving/ad_targeting/models/behavioral/purchase_intent/purchase_intent_model.h"
#include "bat/ads/internal/ad_serving/ad_targeting/models/contextual/text_classification/text_classification_model.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_constants.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_user_model_info.h"
#include "bat/ads/internal/segments/segments_util.h"

namespace ads {
namespace ad_targeting {

namespace {

SegmentList FilterSegments(const SegmentList& segments, const int max_count) {
  SegmentList top_segments;

  int count = 0;

  for (const auto& segment : segments) {
    if (ShouldFilterSegment(segment)) {
      continue;
    }

    top_segments.push_back(segment);
    count++;
    if (count == max_count) {
      break;
    }
  }

  return top_segments;
}

}  // namespace

SegmentList GetTopSegments(const SegmentList& segments,
                           const int max_count,
                           const bool parent_only) {
  if (!parent_only) {
    return FilterSegments(segments, max_count);
  }

  const SegmentList parent_segments = GetParentSegments(segments);
  return FilterSegments(parent_segments, max_count);
}

SegmentList GetTopSegments(const UserModelInfo& user_model,
                           const bool parent_only) {
  SegmentList segments;

  const SegmentList interest_segments = GetTopSegments(
      user_model.interest_segments, kTopInterestSegmentsCount, parent_only);
  segments.insert(segments.end(), interest_segments.cbegin(),
                  interest_segments.cend());

  const SegmentList latent_interest_segments =
      GetTopSegments(user_model.latent_interest_segments,
                     kTopLatentInterestSegmentsCount, parent_only);
  segments.insert(segments.end(), latent_interest_segments.cbegin(),
                  latent_interest_segments.cend());

  const SegmentList purchase_intent_segments =
      GetTopSegments(user_model.purchase_intent_segments,
                     kTopPurchaseIntentSegmentsCount, parent_only);
  segments.insert(segments.end(), purchase_intent_segments.cbegin(),
                  purchase_intent_segments.cend());

  return segments;
}

}  // namespace ad_targeting
}  // namespace ads
