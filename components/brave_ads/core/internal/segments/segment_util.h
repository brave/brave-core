/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SEGMENTS_SEGMENT_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SEGMENTS_SEGMENT_UTIL_H_

#include <string>

#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"

namespace brave_ads {

struct CatalogInfo;

SegmentList GetSegments(const CatalogInfo& catalog);

template <typename T>
SegmentList GetSegments(const T& creative_ads) {
  SegmentList segments;
  segments.reserve(creative_ads.size());

  for (const auto& creative_ad : creative_ads) {
    segments.push_back(creative_ad.segment);
  }

  base::ranges::sort(segments);
  segments.erase(base::ranges::unique(segments), segments.cend());
  return segments;
}

std::string GetParentSegment(const std::string& segment);
SegmentList GetParentSegments(const SegmentList& segments);

bool HasChildSegment(const std::string& segment);

bool ShouldFilterSegment(const std::string& segment);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SEGMENTS_SEGMENT_UTIL_H_
