/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/analytics/p2a/opportunities/p2a_opportunity_util.h"

#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/strings/string_strip_util.h"
#include "brave/components/brave_ads/core/internal/segments/segment_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads::p2a {

namespace {

constexpr char kPerSegmentEventPrefix[] =
    "Brave.P2A.AdOpportunitiesPerSegment.";
constexpr char kTotalEvent[] = "Brave.P2A.TotalAdOpportunities";

std::string NormalizeSegment(const std::string& segment) {
  std::string normalized_segment;
  base::ReplaceChars(StripNonAlphaNumericCharacters(segment), " ", "",
                     &normalized_segment);

  return normalized_segment;
}

absl::optional<std::string> BuildAdOpportunitiesPerSegmentEvent(
    const std::string& segment) {
  const std::string normalized_segment = NormalizeSegment(segment);
  if (segment.empty()) {
    return absl::nullopt;
  }

  return base::StrCat({kPerSegmentEventPrefix, normalized_segment});
}

}  // namespace

std::vector<std::string> BuildAdOpportunityEvents(const SegmentList& segments) {
  std::vector<std::string> events;

  for (const auto& segment : GetParentSegments(segments)) {
    if (const absl::optional<std::string> event =
            BuildAdOpportunitiesPerSegmentEvent(segment)) {
      events.push_back(*event);
    }
  }

  events.emplace_back(kTotalEvent);

  return events;
}

}  // namespace brave_ads::p2a
