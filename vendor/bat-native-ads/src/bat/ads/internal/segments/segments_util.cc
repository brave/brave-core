/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/segments/segments_util.h"

#include <algorithm>
#include <set>
#include <vector>

#include "base/check.h"
#include "base/strings/string_split.h"
#include "bat/ads/internal/catalog/catalog_info.h"
#include "bat/ads/internal/deprecated/client/client_state_manager.h"
#include "bat/ads/internal/deprecated/client/preferences/filtered_category_info.h"

namespace ads {

namespace {

constexpr char kSegmentSeparator[] = "-";

std::vector<std::string> SplitSegment(const std::string& segment) {
  DCHECK(!segment.empty());

  return base::SplitString(segment, kSegmentSeparator, base::KEEP_WHITESPACE,
                           base::SPLIT_WANT_ALL);
}

void RemoveDuplicates(SegmentList* segments) {
  DCHECK(segments);

  std::set<std::string> seen;  // log(n) existence check

  auto iter = segments->begin();
  while (iter != segments->end()) {
    if (seen.find(*iter) != seen.end()) {
      iter = segments->erase(iter);
    } else {
      seen.insert(*iter);
      iter++;
    }
  }
}

}  // namespace

SegmentList GetSegments(const CatalogInfo& catalog) {
  SegmentList segments;

  for (const auto& campaign : catalog.campaigns) {
    for (const auto& creative_set : campaign.creative_sets) {
      for (const auto& segment : creative_set.segments) {
        DCHECK(!segment.name.empty());
        segments.push_back(segment.name);
      }
    }
  }

  RemoveDuplicates(&segments);

  return segments;
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
    DCHECK(!segment.empty());

    const std::string parent_segment = GetParentSegment(segment);
    DCHECK(!parent_segment.empty());

    parent_segments.push_back(parent_segment);
  }

  RemoveDuplicates(&parent_segments);

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

  return GetParentSegment(lhs) == GetParentSegment(rhs);
}

bool ShouldFilterSegment(const std::string& segment) {
  DCHECK(!segment.empty());

  const FilteredCategoryList filtered_segments =
      ClientStateManager::GetInstance()->GetFilteredCategories();

  if (filtered_segments.empty()) {
    return false;
  }

  const auto iter = std::find_if(
      filtered_segments.cbegin(), filtered_segments.cend(),
      [&segment](const FilteredCategoryInfo& filtered_segment) {
        if (HasChildSegment(filtered_segment.name)) {
          // Filter against child, i.e. "technology & computing-linux"
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
