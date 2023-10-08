/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/segments/top_segments.h"

#include "brave/components/brave_ads/core/internal/segments/segment_util.h"

namespace brave_ads {

namespace {

SegmentList FilterTopSegments(const SegmentList& segments,
                              const int max_count) {
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
  return FilterTopSegments(parent_only ? GetParentSegments(segments) : segments,
                           max_count);
}

absl::optional<std::string> GetTopSegment(const SegmentList& segments,
                                          const bool parent_only) {
  const SegmentList top_segments =
      FilterTopSegments(parent_only ? GetParentSegments(segments) : segments,
                        /*max_count=*/1);
  if (top_segments.empty()) {
    return absl::nullopt;
  }

  return top_segments.front();
}

}  // namespace brave_ads
