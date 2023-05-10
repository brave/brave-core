/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/segments/segment_util.h"

#include <vector>

#include "base/check.h"
#include "base/containers/flat_set.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_split.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_info.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/preferences/filtered_category_info.h"

namespace brave_ads {

namespace {

constexpr char kSegmentSeparator[] = "-";

std::vector<std::string> SplitSegment(const std::string& segment) {
  CHECK(!segment.empty());

  return base::SplitString(segment, kSegmentSeparator, base::KEEP_WHITESPACE,
                           base::SPLIT_WANT_ALL);
}

}  // namespace

SegmentList GetSegments(const CatalogInfo& catalog) {
  SegmentList segments;

  base::flat_set<std::string> exists;

  for (const auto& campaign : catalog.campaigns) {
    for (const auto& creative_set : campaign.creative_sets) {
      for (const auto& segment : creative_set.segments) {
        CHECK(!segment.name.empty());

        if (exists.find(segment.name) != std::cend(exists)) {
          continue;
        }

        segments.push_back(segment.name);

        exists.insert(segment.name);
      }
    }
  }

  return segments;
}

std::string GetParentSegment(const std::string& segment) {
  CHECK(!segment.empty());

  const std::vector<std::string> components = SplitSegment(segment);
  CHECK(!components.empty());

  return components.front();
}

SegmentList GetParentSegments(const SegmentList& segments) {
  SegmentList parent_segments;

  base::flat_set<std::string> exists;

  for (const auto& segment : segments) {
    CHECK(!segment.empty());

    const std::string parent_segment = GetParentSegment(segment);
    CHECK(!parent_segment.empty());

    if (exists.find(parent_segment) != std::cend(exists)) {
      continue;
    }

    parent_segments.push_back(parent_segment);

    exists.insert(parent_segment);
  }

  return parent_segments;
}

bool HasChildSegment(const std::string& segment) {
  CHECK(!segment.empty());

  const std::vector<std::string> components = SplitSegment(segment);
  CHECK(!components.empty());

  return components.size() != 1;
}

bool ShouldFilterSegment(const std::string& segment) {
  CHECK(!segment.empty());

  const FilteredCategoryList& filtered_segments =
      ClientStateManager::GetInstance().GetFilteredCategories();

  if (filtered_segments.empty()) {
    return false;
  }

  const auto iter = base::ranges::find_if(
      filtered_segments,
      [&segment](const FilteredCategoryInfo& filtered_segment) {
        if (HasChildSegment(filtered_segment.name)) {
          // Filter against child, i.e. "technology &
          // computing-linux"
          return segment == filtered_segment.name;
        }

        // Filter against parent, i.e. "technology & computing"
        return GetParentSegment(segment) ==
               GetParentSegment(filtered_segment.name);
      });

  return iter != filtered_segments.cend();
}

}  // namespace brave_ads
