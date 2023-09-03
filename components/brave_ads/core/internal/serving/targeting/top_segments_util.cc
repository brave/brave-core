/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/top_segments_util.h"

#include "base/containers/extend.h"
#include "brave/components/brave_ads/core/internal/segments/segment_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model_info.h"

namespace brave_ads {

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
  return FilterSegments(parent_only ? GetParentSegments(segments) : segments,
                        max_count);
}

SegmentList GetTopSegments(const UserModelInfo& user_model,
                           const int max_count,
                           const bool parent_only) {
  SegmentList segments;

  base::Extend(segments, GetTopSegments(user_model.intent_segments, max_count,
                                        parent_only));

  base::Extend(segments, GetTopSegments(user_model.latent_interest_segments,
                                        max_count, parent_only));

  base::Extend(segments, GetTopSegments(user_model.interest_segments, max_count,
                                        parent_only));

  return segments;
}

}  // namespace brave_ads
