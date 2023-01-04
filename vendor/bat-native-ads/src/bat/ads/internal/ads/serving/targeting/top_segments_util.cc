/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/targeting/top_segments_util.h"

#include "bat/ads/internal/ads/serving/targeting/user_model_info.h"
#include "bat/ads/internal/segments/segment_util.h"

namespace ads::targeting {

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
                           const int max_count,
                           const bool parent_only) {
  SegmentList segments;

  const SegmentList interest_segments =
      GetTopSegments(user_model.interest_segments, max_count, parent_only);
  segments.insert(segments.cend(), interest_segments.cbegin(),
                  interest_segments.cend());

  const SegmentList latent_interest_segments = GetTopSegments(
      user_model.latent_interest_segments, max_count, parent_only);
  segments.insert(segments.cend(), latent_interest_segments.cbegin(),
                  latent_interest_segments.cend());

  const SegmentList purchase_intent_segments = GetTopSegments(
      user_model.purchase_intent_segments, max_count, parent_only);
  segments.insert(segments.cend(), purchase_intent_segments.cbegin(),
                  purchase_intent_segments.cend());

  return segments;
}

}  // namespace ads::targeting
