/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pipelines/new_tab_page_ads/eligible_new_tab_page_ads_v1.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/allocation/seen_ads.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/allocation/seen_advertisers.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/eligible_ads_constants.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/eligible_ads_feature.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rules_util.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/new_tab_page_ads/new_tab_page_ad_exclusion_rules.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pacing/pacing.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/priority/priority.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/segments/top_user_model_segments.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_events_database_table.h"

namespace brave_ads {

EligibleNewTabPageAdsV1::EligibleNewTabPageAdsV1(
    const SubdivisionTargeting& subdivision_targeting,
    const AntiTargetingResource& anti_targeting_resource)
    : EligibleNewTabPageAdsBase(subdivision_targeting,
                                anti_targeting_resource) {}

EligibleNewTabPageAdsV1::~EligibleNewTabPageAdsV1() = default;

void EligibleNewTabPageAdsV1::GetForUserModel(
    UserModelInfo user_model,
    EligibleAdsCallback<CreativeNewTabPageAdList> callback) {
  BLOG(1, "Get eligible new tab page ads");

  const database::table::AdEvents database_table;
  database_table.GetForType(
      mojom::AdType::kNewTabPageAd,
      base::BindOnce(
          &EligibleNewTabPageAdsV1::GetEligibleAdsForUserModelCallback,
          weak_factory_.GetWeakPtr(), std::move(user_model),
          std::move(callback)));
}

///////////////////////////////////////////////////////////////////////////////

void EligibleNewTabPageAdsV1::GetEligibleAdsForUserModelCallback(
    UserModelInfo user_model,
    EligibleAdsCallback<CreativeNewTabPageAdList> callback,
    const bool success,
    const AdEventList& ad_events) {
  if (!success) {
    BLOG(1, "Failed to get ad events");
    return std::move(callback).Run(/*eligible_ads=*/{});
  }

  GetBrowsingHistory(
      kBrowsingHistoryMaxCount.Get(), kBrowsingHistoryRecentDayRange.Get(),
      base::BindOnce(&EligibleNewTabPageAdsV1::GetEligibleAds,
                     weak_factory_.GetWeakPtr(), std::move(user_model),
                     ad_events, std::move(callback)));
}

void EligibleNewTabPageAdsV1::GetEligibleAds(
    UserModelInfo user_model,
    const AdEventList& ad_events,
    EligibleAdsCallback<CreativeNewTabPageAdList> callback,
    const BrowsingHistoryList& browsing_history) {
  GetForChildSegments(std::move(user_model), ad_events, browsing_history,
                      std::move(callback));
}

void EligibleNewTabPageAdsV1::GetForChildSegments(
    UserModelInfo user_model,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    EligibleAdsCallback<CreativeNewTabPageAdList> callback) {
  const SegmentList segments = GetTopChildSegments(user_model);
  if (segments.empty()) {
    return GetForParentSegments(user_model, ad_events, browsing_history,
                                std::move(callback));
  }

  BLOG(1, "Get eligible ads for child segments:");
  for (const auto& segment : segments) {
    BLOG(1, "  " << segment);
  }

  const database::table::CreativeNewTabPageAds database_table;
  database_table.GetForSegments(
      segments,
      base::BindOnce(&EligibleNewTabPageAdsV1::GetForChildSegmentsCallback,
                     weak_factory_.GetWeakPtr(), std::move(user_model),
                     ad_events, browsing_history, std::move(callback)));
}

void EligibleNewTabPageAdsV1::GetForChildSegmentsCallback(
    const UserModelInfo& user_model,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    EligibleAdsCallback<CreativeNewTabPageAdList> callback,
    const bool success,
    const SegmentList& /*segments=*/,
    const CreativeNewTabPageAdList& creative_ads) {
  if (!success) {
    BLOG(1, "Failed to get ads for child segments");
    return std::move(callback).Run(/*eligible_ads=*/{});
  }

  const CreativeNewTabPageAdList eligible_creative_ads =
      FilterCreativeAds(creative_ads, ad_events, browsing_history);
  if (eligible_creative_ads.empty()) {
    BLOG(1, "No eligible ads out of " << creative_ads.size()
                                      << " ads for child segments");
    return GetForParentSegments(user_model, ad_events, browsing_history,
                                std::move(callback));
  }

  BLOG(1, eligible_creative_ads.size()
              << " eligible ads out of " << creative_ads.size()
              << " ads for child segments");

  std::move(callback).Run(eligible_creative_ads);
}

void EligibleNewTabPageAdsV1::GetForParentSegments(
    const UserModelInfo& user_model,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    EligibleAdsCallback<CreativeNewTabPageAdList> callback) {
  const SegmentList segments = GetTopParentSegments(user_model);
  if (segments.empty()) {
    return GetForUntargeted(ad_events, browsing_history, std::move(callback));
  }

  BLOG(1, "Get eligible ads for parent segments:");
  for (const auto& segment : segments) {
    BLOG(1, "  " << segment);
  }

  const database::table::CreativeNewTabPageAds database_table;
  database_table.GetForSegments(
      segments,
      base::BindOnce(&EligibleNewTabPageAdsV1::GetForParentSegmentsCallback,
                     weak_factory_.GetWeakPtr(), ad_events, browsing_history,
                     std::move(callback)));
}

void EligibleNewTabPageAdsV1::GetForParentSegmentsCallback(
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    EligibleAdsCallback<CreativeNewTabPageAdList> callback,
    const bool success,
    const SegmentList& /*segments=*/,
    const CreativeNewTabPageAdList& creative_ads) {
  if (!success) {
    BLOG(1, "Failed to get ads for parent segments");
    return std::move(callback).Run(/*eligible_ads=*/{});
  }

  const CreativeNewTabPageAdList eligible_creative_ads =
      FilterCreativeAds(creative_ads, ad_events, browsing_history);
  if (eligible_creative_ads.empty()) {
    BLOG(1, "No eligible ads out of " << creative_ads.size()
                                      << " ads for parent segments");
    return GetForUntargeted(ad_events, browsing_history, std::move(callback));
  }

  BLOG(1, eligible_creative_ads.size()
              << " eligible ads out of " << creative_ads.size()
              << " ads for parent segments");

  std::move(callback).Run(eligible_creative_ads);
}

void EligibleNewTabPageAdsV1::GetForUntargeted(
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    EligibleAdsCallback<CreativeNewTabPageAdList> callback) {
  BLOG(1, "Get eligible ads for untargeted segment");

  const database::table::CreativeNewTabPageAds database_table;
  database_table.GetForSegments(
      {kUntargeted},
      base::BindOnce(&EligibleNewTabPageAdsV1::GetForUntargetedCallback,
                     weak_factory_.GetWeakPtr(), ad_events, browsing_history,
                     std::move(callback)));
}

void EligibleNewTabPageAdsV1::GetForUntargetedCallback(
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    EligibleAdsCallback<CreativeNewTabPageAdList> callback,
    const bool success,
    const SegmentList& /*segments=*/,
    const CreativeNewTabPageAdList& creative_ads) {
  if (!success) {
    BLOG(1, "Failed to get ads for untargeted segment");
    return std::move(callback).Run(/*eligible_ads=*/{});
  }

  const CreativeNewTabPageAdList eligible_creative_ads =
      FilterCreativeAds(creative_ads, ad_events, browsing_history);
  if (eligible_creative_ads.empty()) {
    BLOG(1, "No eligible ads out of " << creative_ads.size()
                                      << " ads for untargeted segment");
    return std::move(callback).Run(/*eligible_ads=*/{});
  }

  std::move(callback).Run(eligible_creative_ads);
}

CreativeNewTabPageAdList EligibleNewTabPageAdsV1::FilterCreativeAds(
    const CreativeNewTabPageAdList& creative_ads,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history) {
  if (creative_ads.empty()) {
    return {};
  }

  NewTabPageAdExclusionRules exclusion_rules(ad_events, *subdivision_targeting_,
                                             *anti_targeting_resource_,
                                             browsing_history);

  CreativeNewTabPageAdList eligible_creative_ads =
      ApplyExclusionRules(creative_ads, last_served_ad_, &exclusion_rules);

  eligible_creative_ads = FilterSeenAdvertisersAndRoundRobinIfNeeded(
      eligible_creative_ads, AdType::kNewTabPageAd);

  eligible_creative_ads = FilterSeenAdsAndRoundRobinIfNeeded(
      eligible_creative_ads, AdType::kNewTabPageAd);

  eligible_creative_ads = PaceCreativeAds(eligible_creative_ads);

  return PrioritizeCreativeAds(eligible_creative_ads);
}

}  // namespace brave_ads
