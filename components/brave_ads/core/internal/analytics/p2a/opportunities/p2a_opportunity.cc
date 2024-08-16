/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/analytics/p2a/opportunities/p2a_opportunity.h"

#include <cstddef>

#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/analytics/p2a/opportunities/p2a_opportunity_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/segments/top_segments.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"

namespace brave_ads {

namespace {
constexpr size_t kTopSegmentsMaxCount = 1;
}  // namespace

void RecordP2AAdOpportunity(const AdType ad_type, const SegmentList& segments) {
  const SegmentList top_segments =
      GetTopSegments(segments, kTopSegmentsMaxCount, /*parent_only=*/false);

  GetAdsClient()->RecordP2AEvents(
      BuildP2AAdOpportunityEvents(ad_type, top_segments));
}

}  // namespace brave_ads
