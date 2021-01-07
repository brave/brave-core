/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/ad_targeting_segment_util.h"

#include "base/strings/string_split.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/client/preferences/filtered_category_info.h"

namespace ads {

namespace {
const char kSegmentSeparator[] = "-";
}  // namespace

std::vector<std::string> SplitSegment(
    const std::string& segment) {
  return base::SplitString(segment, kSegmentSeparator, base::KEEP_WHITESPACE,
      base::SPLIT_WANT_ALL);
}

std::string GetParentSegment(
    const std::string& segment) {
  std::string parent_segment;
  if (segment.empty()) {
    return parent_segment;
  }

  const std::vector<std::string> components = SplitSegment(segment);
  parent_segment = components.front();

  return parent_segment;
}

SegmentList GetParentSegments(
    const SegmentList& segments) {
  SegmentList parent_segments;

  for (const auto& segment : segments) {
    const std::string parent_segment = GetParentSegment(segment);
    if (std::find(parent_segments.begin(), parent_segments.end(),
        parent_segment) != parent_segments.end()) {
      continue;
    }

    parent_segments.push_back(parent_segment);
  }

  return parent_segments;
}

bool ShouldFilterSegment(
    const std::string& segment) {
  // If passed in segment has a sub segment and the current filter does not,
  // check if it's a child of the filter. Conversely, if the passed in segment
  // has no sub segment but the current filter does, it can't be a match at all
  // so move on to the next filter. Otherwise, perform an exact match to
  // determine whether or not to filter the segment

  const std::vector<std::string> segment_components = SplitSegment(segment);

  const FilteredCategoryList filtered_segments =
      Client::Get()->get_filtered_categories();

  for (const auto& filtered_segment : filtered_segments) {
    const std::vector<std::string> filtered_segment_components =
        SplitSegment(filtered_segment.name);

    if (segment_components.size() > 1 &&
        filtered_segment_components.size() == 1) {
      if (segment_components.front() ==
          filtered_segment_components.front()) {
        return true;
      }
    } else if (segment_components.size() == 1 &&
        filtered_segment_components.size() > 1) {
      continue;
    } else if (filtered_segment.name == segment) {
      return true;
    }
  }

  return false;
}

}  // namespace ads
