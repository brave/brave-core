/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/analytics/p2a/opportunities/p2a_opportunity_util.h"

#include <optional>

#include "base/check.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/strings/string_strip_util.h"
#include "brave/components/brave_ads/core/internal/segments/segment_util.h"

namespace brave_ads {

namespace {

constexpr char kAdOpportunitiesPerSegmentEvent[] =
    "Brave.P2A.$1.opportunities_per_segment.$2";
constexpr char kAdOpportunitiesEvent[] = "Brave.P2A.$1.opportunities";

std::optional<std::string> NormalizeSegment(const std::string& segment) {
  std::string normalized_segment;
  base::ReplaceChars(StripNonAlphaNumericCharacters(segment), " ", "",
                     &normalized_segment);

  if (normalized_segment.empty()) {
    return std::nullopt;
  }

  return normalized_segment;
}

std::optional<std::string> BuildAdOpportunitiesPerSegmentEvent(
    const AdType ad_type,
    const std::string& segment) {
  CHECK_NE(AdType::kUndefined, ad_type);
  CHECK(!segment.empty());

  const std::string parent_segment = GetParentSegment(segment);
  const std::optional<std::string> normalized_segment =
      NormalizeSegment(parent_segment);
  if (!normalized_segment) {
    return std::nullopt;
  }

  return base::ReplaceStringPlaceholders(
      kAdOpportunitiesPerSegmentEvent, {ToString(ad_type), *normalized_segment},
      nullptr);
}

std::string BuildAdOpportunitiesEvent(const AdType ad_type) {
  CHECK_NE(AdType::kUndefined, ad_type);

  return base::ReplaceStringPlaceholders(kAdOpportunitiesEvent,
                                         {ToString(ad_type)}, nullptr);
}

}  // namespace

std::vector<std::string> BuildP2AAdOpportunityEvents(
    const AdType ad_type,
    const SegmentList& segments) {
  CHECK_NE(AdType::kUndefined, ad_type);

  std::vector<std::string> events;
  events.reserve(segments.size() + 1);

  for (const auto& segment : segments) {
    if (const std::optional<std::string> ad_opportunities_per_segment_event =
            BuildAdOpportunitiesPerSegmentEvent(ad_type, segment)) {
      events.push_back(*ad_opportunities_per_segment_event);
    }
  }

  events.push_back(BuildAdOpportunitiesEvent(ad_type));

  return events;
}

}  // namespace brave_ads
