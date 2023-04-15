/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/pipelines/inline_content_ads/eligible_inline_content_ads_v1.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom-shared.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/allocation/seen_ads.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/allocation/seen_advertisers.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/eligible_ads_constants.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/eligible_ads_features.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rules_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/inline_content_ads/inline_content_ad_exclusion_rules.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/pacing/pacing.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/priority/priority.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/top_segments.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_info.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"

namespace brave_ads::inline_content_ads {

EligibleAdsV1::EligibleAdsV1(
    const geographic::SubdivisionTargeting& subdivision_targeting,
    const resource::AntiTargeting& anti_targeting_resource)
    : EligibleAdsBase(subdivision_targeting, anti_targeting_resource) {}

EligibleAdsV1::~EligibleAdsV1() = default;

void EligibleAdsV1::GetForUserModel(
    targeting::UserModelInfo user_model,
    const std::string& dimensions,
    GetEligibleAdsCallback<CreativeInlineContentAdList> callback) {
  BLOG(1, "Get eligible inline content ads:");

  const database::table::AdEvents database_table;
  database_table.GetForType(
      mojom::AdType::kInlineContentAd,
      base::BindOnce(&EligibleAdsV1::OnGetForUserModel,
                     weak_factory_.GetWeakPtr(), std::move(user_model),
                     dimensions, std::move(callback)));
}

void EligibleAdsV1::OnGetForUserModel(
    targeting::UserModelInfo user_model,
    const std::string& dimensions,
    GetEligibleAdsCallback<CreativeInlineContentAdList> callback,
    const bool success,
    const AdEventList& ad_events) {
  if (!success) {
    BLOG(1, "Failed to get ad events");
    return std::move(callback).Run(/*had_opportunity*/ false,
                                   /*eligible_ads*/ {});
  }

  GetBrowsingHistory(std::move(user_model), dimensions, ad_events,
                     std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////

void EligibleAdsV1::GetBrowsingHistory(
    targeting::UserModelInfo user_model,
    const std::string& dimensions,
    const AdEventList& ad_events,
    GetEligibleAdsCallback<CreativeInlineContentAdList> callback) {
  AdsClientHelper::GetInstance()->GetBrowsingHistory(
      kBrowsingHistoryMaxCount.Get(), kBrowsingHistoryDaysAgo.Get(),
      base::BindOnce(&EligibleAdsV1::GetEligibleAds, weak_factory_.GetWeakPtr(),
                     std::move(user_model), dimensions, ad_events,
                     std::move(callback)));
}

void EligibleAdsV1::GetEligibleAds(
    targeting::UserModelInfo user_model,
    const std::string& dimensions,
    const AdEventList& ad_events,
    GetEligibleAdsCallback<CreativeInlineContentAdList> callback,
    const BrowsingHistoryList& browsing_history) {
  GetForChildSegments(std::move(user_model), dimensions, ad_events,
                      browsing_history, std::move(callback));
}

void EligibleAdsV1::GetForChildSegments(
    targeting::UserModelInfo user_model,
    const std::string& dimensions,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    GetEligibleAdsCallback<CreativeInlineContentAdList> callback) {
  const SegmentList segments = targeting::GetTopChildSegments(user_model);
  if (segments.empty()) {
    return GetForParentSegments(user_model, dimensions, ad_events,
                                browsing_history, std::move(callback));
  }

  BLOG(1, "Get eligible ads for child segments:");
  for (const auto& segment : segments) {
    BLOG(1, "  " << segment);
  }

  const database::table::CreativeInlineContentAds database_table;
  database_table.GetForSegmentsAndDimensions(
      segments, dimensions,
      base::BindOnce(&EligibleAdsV1::OnGetForChildSegments,
                     weak_factory_.GetWeakPtr(), std::move(user_model),
                     dimensions, ad_events, browsing_history,
                     std::move(callback)));
}

void EligibleAdsV1::OnGetForChildSegments(
    const targeting::UserModelInfo& user_model,
    const std::string& dimensions,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    GetEligibleAdsCallback<CreativeInlineContentAdList> callback,
    const bool success,
    const SegmentList& /*segments*/,
    const CreativeInlineContentAdList& creative_ads) {
  if (!success) {
    BLOG(1, "Failed to get ads for child segments");
    return std::move(callback).Run(/*had_opportunity*/ false,
                                   /*eligible_ads*/ {});
  }

  const CreativeInlineContentAdList eligible_creative_ads =
      FilterCreativeAds(creative_ads, ad_events, browsing_history);
  if (eligible_creative_ads.empty()) {
    BLOG(1, "No eligible ads out of " << creative_ads.size()
                                      << " ads for child segments");
    return GetForParentSegments(user_model, dimensions, ad_events,
                                browsing_history, std::move(callback));
  }

  BLOG(1, eligible_creative_ads.size()
              << " eligible ads out of " << creative_ads.size()
              << " ads for child segments");

  std::move(callback).Run(/*had_opportunity*/ true, eligible_creative_ads);
}

void EligibleAdsV1::GetForParentSegments(
    const targeting::UserModelInfo& user_model,
    const std::string& dimensions,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    GetEligibleAdsCallback<CreativeInlineContentAdList> callback) {
  const SegmentList segments = targeting::GetTopParentSegments(user_model);
  if (segments.empty()) {
    return GetForUntargeted(dimensions, ad_events, browsing_history,
                            std::move(callback));
  }

  BLOG(1, "Get eligible ads for parent segments:");
  for (const auto& segment : segments) {
    BLOG(1, "  " << segment);
  }

  const database::table::CreativeInlineContentAds database_table;
  database_table.GetForSegmentsAndDimensions(
      segments, dimensions,
      base::BindOnce(&EligibleAdsV1::OnGetForParentSegments,
                     weak_factory_.GetWeakPtr(), dimensions, ad_events,
                     browsing_history, std::move(callback)));
}

void EligibleAdsV1::OnGetForParentSegments(
    const std::string& dimensions,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    GetEligibleAdsCallback<CreativeInlineContentAdList> callback,
    const bool success,
    const SegmentList& /*segments*/,
    const CreativeInlineContentAdList& creative_ads) {
  if (!success) {
    BLOG(1, "Failed to get ads for parent segments");
    return std::move(callback).Run(/*had_opportunity*/ false,
                                   /*eligible_ads*/ {});
  }

  const CreativeInlineContentAdList eligible_creative_ads =
      FilterCreativeAds(creative_ads, ad_events, browsing_history);
  if (eligible_creative_ads.empty()) {
    BLOG(1, "No eligible ads out of " << creative_ads.size()
                                      << " ads for parent segments");
    return GetForUntargeted(dimensions, ad_events, browsing_history,
                            std::move(callback));
  }

  BLOG(1, eligible_creative_ads.size()
              << " eligible ads out of " << creative_ads.size()
              << " ads for parent segments");

  std::move(callback).Run(/*had_opportunity*/ true, eligible_creative_ads);
}

void EligibleAdsV1::GetForUntargeted(
    const std::string& dimensions,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    GetEligibleAdsCallback<CreativeInlineContentAdList> callback) {
  BLOG(1, "Get eligible ads for untargeted segment");

  const database::table::CreativeInlineContentAds database_table;
  database_table.GetForSegmentsAndDimensions(
      {kUntargeted}, dimensions,
      base::BindOnce(&EligibleAdsV1::OnGetForUntargeted,
                     weak_factory_.GetWeakPtr(), ad_events, browsing_history,
                     std::move(callback)));
}

void EligibleAdsV1::OnGetForUntargeted(
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    GetEligibleAdsCallback<CreativeInlineContentAdList> callback,
    const bool success,
    const SegmentList& /*segments*/,
    const CreativeInlineContentAdList& creative_ads) {
  if (!success) {
    BLOG(1, "Failed to get ads for untargeted segment");
    return std::move(callback).Run(/*had_opportunity*/ false,
                                   /*eligible_ads*/ {});
  }

  const CreativeInlineContentAdList eligible_creative_ads =
      FilterCreativeAds(creative_ads, ad_events, browsing_history);
  if (eligible_creative_ads.empty()) {
    BLOG(1, "No eligible ads out of " << creative_ads.size()
                                      << " ads for untargeted segment");
    return std::move(callback).Run(/*had_opportunity*/ false,
                                   /*eligible_ads*/ {});
  }

  BLOG(1, eligible_creative_ads.size()
              << " eligible ads out of " << creative_ads.size()
              << " ads for untargeted segment");

  std::move(callback).Run(/*had_opportunity*/ true, eligible_creative_ads);
}

CreativeInlineContentAdList EligibleAdsV1::FilterCreativeAds(
    const CreativeInlineContentAdList& creative_ads,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history) {
  if (creative_ads.empty()) {
    return {};
  }

  CreativeInlineContentAdList eligible_creative_ads = creative_ads;

  ExclusionRules exclusion_rules(ad_events, *subdivision_targeting_,
                                 *anti_targeting_resource_, browsing_history);
  eligible_creative_ads = ApplyExclusionRules(
      eligible_creative_ads, last_served_ad_, &exclusion_rules);

  eligible_creative_ads = FilterSeenAdvertisersAndRoundRobinIfNeeded(
      eligible_creative_ads, AdType::kInlineContentAd);

  eligible_creative_ads = FilterSeenAdsAndRoundRobinIfNeeded(
      eligible_creative_ads, AdType::kInlineContentAd);

  eligible_creative_ads = PaceCreativeAds(eligible_creative_ads);

  return PrioritizeCreativeAds(eligible_creative_ads);
}

}  // namespace brave_ads::inline_content_ads
