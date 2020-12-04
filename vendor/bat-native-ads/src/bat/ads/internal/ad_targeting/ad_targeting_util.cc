/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/ad_targeting_util.h"

#include "base/strings/string_split.h"

namespace ads {
namespace ad_targeting {

namespace {
const char kSegmentSeparator[] = "-";
}  // namespace

std::vector<std::string> SplitSegment(
    const std::string& segment) {
  return base::SplitString(segment, kSegmentSeparator, base::KEEP_WHITESPACE,
      base::SPLIT_WANT_ALL);
}

SegmentList GetParentSegments(
    const SegmentList& segments) {
  SegmentList parent_segments;

  for (const auto& segment : segments) {
    const std::vector<std::string> components = SplitSegment(segment);

    const std::string parent_segment = components.front();
    if (std::find(parent_segments.begin(), parent_segments.end(),
        parent_segment) != parent_segments.end()) {
      continue;
    }

    parent_segments.push_back(parent_segment);
  }

  return parent_segments;
}

}  // namespace ad_targeting
}  // namespace ads
