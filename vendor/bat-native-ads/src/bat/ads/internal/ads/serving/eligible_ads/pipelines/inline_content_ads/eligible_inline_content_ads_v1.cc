/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/pipelines/inline_content_ads/eligible_inline_content_ads_v1.h"

#include <utility>

#include "base/functional/bind.h"
#include "bat/ads/internal/ads/ad_events/ad_events_database_table.h"
#include "bat/ads/internal/ads/serving/eligible_ads/allocation/seen_ads.h"
#include "bat/ads/internal/ads/serving/eligible_ads/allocation/seen_advertisers.h"
#include "bat/ads/internal/ads/serving/eligible_ads/eligible_ads_constants.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rules_util.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/inline_content_ads/inline_content_ad_exclusion_rules.h"
#include "bat/ads/internal/ads/serving/eligible_ads/pacing/pacing.h"
#include "bat/ads/internal/ads/serving/eligible_ads/priority/priority.h"
#include "bat/ads/internal/ads/serving/serving_features.h"
#include "bat/ads/internal/ads/serving/targeting/top_segments.h"
#include "bat/ads/internal/ads/serving/targeting/user_model_info.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ads_database_table.h"
#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "bat/ads/public/interfaces/ads.mojom-shared.h"

namespace ads::inline_content_ads {

EligibleAdsV1::EligibleAdsV1(
    geographic::SubdivisionTargeting* subdivision_targeting,
    resource::AntiTargeting* anti_targeting_resource)
    : EligibleAdsBase(subdivision_targeting, anti_targeting_resource) {}

void EligibleAdsV1::GetForUserModel(
    targeting::UserModelInfo user_model,
    const std::string& dimensions,
    GetEligibleAdsCallback<CreativeInlineContentAdList> callback) {
  BLOG(1, "Get eligible inline content ads:");

  const database::table::AdEvents database_table;
  database_table.GetForType(
      mojom::AdType::kInlineContentAd,
      base::BindOnce(&EligibleAdsV1::OnGetForUserModel, base::Unretained(this),
                     std::move(user_model), dimensions, std::move(callback)));
}

void EligibleAdsV1::OnGetForUserModel(
    targeting::UserModelInfo user_model,
    const std::string& dimensions,
    GetEligibleAdsCallback<CreativeInlineContentAdList> callback,
    const bool success,
    const AdEventList& ad_events) {
  if (!success) {
    BLOG(1, "Failed to get ad events");
    std::move(callback).Run(/*had_opportunity*/ false, {});
    return;
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
  const int max_count = features::GetBrowsingHistoryMaxCount();
  const int days_ago = features::GetBrowsingHistoryDaysAgo();
  AdsClientHelper::GetInstance()->GetBrowsingHistory(
      max_count, days_ago,
      base::BindOnce(&EligibleAdsV1::GetEligibleAds, base::Unretained(this),
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
    GetForParentSegments(user_model, dimensions, ad_events, browsing_history,
                         std::move(callback));
    return;
  }

  BLOG(1, "Get eligible ads for child segments:");
  for (const auto& segment : segments) {
    BLOG(1, "  " << segment);
  }

  const database::table::CreativeInlineContentAds database_table;
  database_table.GetForSegmentsAndDimensions(
      segments, dimensions,
      base::BindOnce(&EligibleAdsV1::OnGetForChildSegments,
                     base::Unretained(this), std::move(user_model), dimensions,
                     ad_events, browsing_history, std::move(callback)));
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
    std::move(callback).Run(/*had_opportunity*/ false, {});
    return;
  }

  const CreativeInlineContentAdList eligible_creative_ads =
      FilterCreativeAds(creative_ads, ad_events, browsing_history);
  if (eligible_creative_ads.empty()) {
    BLOG(1, "No eligible ads out of " << creative_ads.size()
                                      << " ads for child segments");
    GetForParentSegments(user_model, dimensions, ad_events, browsing_history,
                         std::move(callback));
    return;
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
    GetForUntargeted(dimensions, ad_events, browsing_history,
                     std::move(callback));
    return;
  }

  BLOG(1, "Get eligible ads for parent segments:");
  for (const auto& segment : segments) {
    BLOG(1, "  " << segment);
  }

  const database::table::CreativeInlineContentAds database_table;
  database_table.GetForSegmentsAndDimensions(
      segments, dimensions,
      base::BindOnce(&EligibleAdsV1::OnGetForParentSegments,
                     base::Unretained(this), dimensions, ad_events,
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
    std::move(callback).Run(/*had_opportunity*/ false, {});
    return;
  }

  const CreativeInlineContentAdList eligible_creative_ads =
      FilterCreativeAds(creative_ads, ad_events, browsing_history);
  if (eligible_creative_ads.empty()) {
    BLOG(1, "No eligible ads out of " << creative_ads.size()
                                      << " ads for parent segments");
    GetForUntargeted(dimensions, ad_events, browsing_history,
                     std::move(callback));
    return;
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
      base::BindOnce(&EligibleAdsV1::OnGetForUntargeted, base::Unretained(this),
                     ad_events, browsing_history, std::move(callback)));
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
    std::move(callback).Run(/*had_opportunity*/ false, {});
    return;
  }

  const CreativeInlineContentAdList eligible_creative_ads =
      FilterCreativeAds(creative_ads, ad_events, browsing_history);
  if (eligible_creative_ads.empty()) {
    BLOG(1, "No eligible ads out of " << creative_ads.size()
                                      << " ads for untargeted segment");
    std::move(callback).Run(/*had_opportunity*/ false, {});
    return;
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

  ExclusionRules exclusion_rules(ad_events, subdivision_targeting_,
                                 anti_targeting_resource_, browsing_history);
  eligible_creative_ads = ApplyExclusionRules(
      eligible_creative_ads, last_served_ad_, &exclusion_rules);

  eligible_creative_ads = FilterSeenAdvertisersAndRoundRobinIfNeeded(
      eligible_creative_ads, AdType::kInlineContentAd);

  eligible_creative_ads = FilterSeenAdsAndRoundRobinIfNeeded(
      eligible_creative_ads, AdType::kInlineContentAd);

  eligible_creative_ads = PaceCreativeAds(eligible_creative_ads);

  eligible_creative_ads = PrioritizeCreativeAds(eligible_creative_ads);

  return eligible_creative_ads;
}

}  // namespace ads::inline_content_ads
