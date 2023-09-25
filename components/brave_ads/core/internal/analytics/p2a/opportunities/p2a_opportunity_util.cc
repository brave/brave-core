/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/analytics/p2a/opportunities/p2a_opportunity_util.h"

#include "base/check.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/strings/string_strip_util.h"
#include "brave/components/brave_ads/core/internal/segments/segment_util.h"
#include "brave/components/brave_ads/core/public/units/ad_type.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

namespace {

constexpr char kAdOpportunitiesPerSegmentEvent[] =
    "Brave.P2A.$1.opportunities_per_segment.$2";
constexpr char kAdOpportunitiesEvent[] = "Brave.P2A.$1.opportunities";

std::string NormalizeSegment(const std::string& segment) {
  std::string normalized_segment;
  base::ReplaceChars(StripNonAlphaNumericCharacters(segment), " ", "",
                     &normalized_segment);
  return normalized_segment;
}

absl::optional<std::string> BuildAdOpportunitiesPerSegmentEvent(
    const AdType& ad_type,
    const std::string& segment) {
  CHECK_NE(AdType::kUndefined, ad_type);
  CHECK(!segment.empty());

  const std::string parent_segment = GetParentSegment(segment);
  const std::string normalized_segment = NormalizeSegment(parent_segment);
  if (normalized_segment.empty()) {
    return absl::nullopt;
  }

  return base::ReplaceStringPlaceholders(
      kAdOpportunitiesPerSegmentEvent, {ad_type.ToString(), normalized_segment},
      nullptr);
}

std::string BuildAdOpportunitiesEvent(const AdType& ad_type) {
  CHECK_NE(AdType::kUndefined, ad_type);

  return base::ReplaceStringPlaceholders(kAdOpportunitiesEvent,
                                         {ad_type.ToString()}, nullptr);
}

}  // namespace

std::vector<std::string> BuildP2AAdOpportunityEvents(
    const AdType& ad_type,
    const SegmentList& segments) {
  CHECK_NE(AdType::kUndefined, ad_type);

  std::vector<std::string> events;

  for (const auto& segment : segments) {
    if (const absl::optional<std::string> ad_opportunities_per_segment_event =
            BuildAdOpportunitiesPerSegmentEvent(ad_type, segment)) {
      events.push_back(*ad_opportunities_per_segment_event);
    }
  }

  const std::string ad_opportunities_event = BuildAdOpportunitiesEvent(ad_type);
  events.push_back(ad_opportunities_event);

  return events;
}

}  // namespace brave_ads
