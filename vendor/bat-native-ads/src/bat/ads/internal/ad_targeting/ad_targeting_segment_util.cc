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

std::vector<std::string> SplitSegment(const std::string& segment) {
  DCHECK(!segment.empty());

  return base::SplitString(segment, kSegmentSeparator, base::KEEP_WHITESPACE,
                           base::SPLIT_WANT_ALL);
}

std::string GetParentSegment(const std::string& segment) {
  DCHECK(!segment.empty());

  const std::vector<std::string> components = SplitSegment(segment);
  DCHECK(!components.empty());

  return components.front();
}

SegmentList GetParentSegments(const SegmentList& segments) {
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

bool HasChildSegment(const std::string& segment) {
  DCHECK(!segment.empty());

  const std::vector<std::string> components = SplitSegment(segment);
  DCHECK(!components.empty());

  if (components.size() == 1) {
    return false;
  }

  return true;
}

bool ParentSegmentsMatch(const std::string& lhs, const std::string& rhs) {
  DCHECK(!lhs.empty());
  DCHECK(!rhs.empty());

  const std::vector<std::string> lhs_segment_components = SplitSegment(lhs);
  DCHECK(!lhs_segment_components.empty());
  const std::string lhs_parent_segment = lhs_segment_components.front();

  const std::vector<std::string> rhs_segment_components = SplitSegment(rhs);
  DCHECK(!rhs_segment_components.empty());
  const std::string rhs_parent_segment = rhs_segment_components.front();

  return lhs_parent_segment == rhs_parent_segment;
}

bool ShouldFilterSegment(const std::string& segment) {
  DCHECK(!segment.empty());

  const FilteredCategoryList filtered_segments =
      Client::Get()->get_filtered_categories();

  const auto iter = std::find_if(
      filtered_segments.begin(), filtered_segments.end(),
      [&segment](const FilteredCategoryInfo& filtered_segment) {
        if (HasChildSegment(filtered_segment.name)) {
          // Filter against parent-child, i.e. "technology & computing-linux"
          return segment == filtered_segment.name;
        } else {
          // Filter against parent, i.e. "technology & computing"
          return ParentSegmentsMatch(segment, filtered_segment.name);
        }
      });

  if (iter == filtered_segments.end()) {
    return false;
  }

  return true;
}

}  // namespace ads
