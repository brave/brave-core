/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/segments/top_segments.h"

#include "brave/components/brave_ads/core/internal/segments/segment_util.h"

namespace brave_ads {

namespace {

SegmentList FilterTopSegments(const SegmentList& segments, size_t max_count) {
  SegmentList top_segments;
  top_segments.reserve(max_count);

  for (const auto& segment : segments) {
    if (ShouldFilterSegment(segment)) {
      continue;
    }

    top_segments.push_back(segment);
    if (top_segments.size() == max_count) {
      break;
    }
  }

  return top_segments;
}

}  // namespace

SegmentList GetTopSegments(const SegmentList& segments,
                           size_t max_count,
                           bool parent_only) {
  return FilterTopSegments(parent_only ? GetParentSegments(segments) : segments,
                           max_count);
}

std::optional<std::string> GetTopSegment(const SegmentList& segments,
                                         bool parent_only) {
  const SegmentList& target_segments =
      parent_only ? GetParentSegments(segments) : segments;
  const auto iter = base::ranges::find_if(
      target_segments,
      [](const auto& segment) { return !ShouldFilterSegment(segment); });

  if (iter == target_segments.cend()) {
    return std::nullopt;
  }

  return *iter;
}

}  // namespace brave_ads
