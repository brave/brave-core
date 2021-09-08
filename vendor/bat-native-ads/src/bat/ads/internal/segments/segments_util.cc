/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/segments/segments_util.h"

#include <set>
#include <vector>

#include "base/check.h"
#include "base/strings/string_split.h"
#include "bat/ads/internal/catalog/catalog.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/client/preferences/filtered_category_info.h"

namespace ads {

namespace {

const char kSegmentSeparator[] = "-";

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

SegmentList GetSegments(const Catalog& catalog) {
  SegmentList segments;

  const CatalogCampaignList catalog_campaigns = catalog.GetCampaigns();
  for (const auto& catalog_campaign : catalog_campaigns) {
    CatalogCreativeSetList catalog_creative_sets =
        catalog_campaign.creative_sets;
    for (const auto& catalog_creative_set : catalog_creative_sets) {
      CatalogSegmentList catalog_segments = catalog_creative_set.segments;
      for (const auto& catalog_segment : catalog_segments) {
        segments.push_back(catalog_segment.name);
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
    const std::string parent_segment = GetParentSegment(segment);
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
      Client::Get()->get_filtered_categories();

  if (filtered_segments.empty()) {
    return false;
  }

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
