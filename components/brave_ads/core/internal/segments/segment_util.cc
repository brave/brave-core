/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/segments/segment_util.h"

#include <utility>
#include <vector>

#include "base/check.h"
#include "base/containers/contains.h"
#include "base/containers/flat_set.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_split.h"
#include "brave/components/brave_ads/core/internal/ads_core/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_info.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"

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

        const auto [_, inserted] = exists.insert(segment.name);
        if (inserted) {
          segments.push_back(segment.name);
        }
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

    if (base::Contains(exists, parent_segment)) {
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

  const ReactionMap& segment_reactions = GetReactions().Segments();

  {
    // Filter matching segment, i.e. "technology & computing-linux".
    const auto iter = base::ranges::find(
        segment_reactions, segment,
        &std::pair<const std::string, mojom::ReactionType>::first);
    if (iter != segment_reactions.cend() &&
        iter->second == mojom::ReactionType::kDisliked) {
      return true;
    }
  }

  {
    // Filter matching parent segment, i.e. "technology & computing".
    const std::string parent_segment = GetParentSegment(segment);
    const auto iter = base::ranges::find(
        segment_reactions, parent_segment,
        &std::pair<const std::string, mojom::ReactionType>::first);
    if (iter != segment_reactions.cend() &&
        iter->second == mojom::ReactionType::kDisliked) {
      return true;
    }
  }

  return false;
}

}  // namespace brave_ads
