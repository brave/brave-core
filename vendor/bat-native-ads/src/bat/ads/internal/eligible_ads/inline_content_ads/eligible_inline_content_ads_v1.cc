/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/inline_content_ads/eligible_inline_content_ads_v1.h"

#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ad_pacing/ad_pacing.h"
#include "bat/ads/internal/ad_priority/ad_priority.h"
#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/ad_targeting/ad_targeting.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_user_model_info.h"
#include "bat/ads/internal/ads/inline_content_ads/inline_content_ad_exclusion_rules.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/database/tables/ad_events_database_table.h"
#include "bat/ads/internal/database/tables/creative_inline_content_ads_database_table.h"
#include "bat/ads/internal/eligible_ads/eligible_ads_constants.h"
#include "bat/ads/internal/eligible_ads/eligible_ads_util.h"
#include "bat/ads/internal/eligible_ads/seen_ads.h"
#include "bat/ads/internal/eligible_ads/seen_advertisers.h"
#include "bat/ads/internal/features/ad_serving/ad_serving_features.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"

namespace ads {
namespace inline_content_ads {

EligibleAdsV1::EligibleAdsV1(
    ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
    resource::AntiTargeting* anti_targeting_resource)
    : EligibleAdsBase(subdivision_targeting, anti_targeting_resource) {}

EligibleAdsV1::~EligibleAdsV1() = default;

void EligibleAdsV1::GetForUserModel(
    const ad_targeting::UserModelInfo& user_model,
    const std::string& dimensions,
    GetEligibleAdsCallback callback) {
  BLOG(1, "Get eligible inline content ads:");

  database::table::AdEvents database_table;
  database_table.GetAll([=](const bool success, const AdEventList& ad_events) {
    if (!success) {
      BLOG(1, "Failed to get ad events");
      callback(/* had_opportunity */ false, {});
      return;
    }

    const int max_count = features::GetBrowsingHistoryMaxCount();
    const int days_ago = features::GetBrowsingHistoryDaysAgo();
    AdsClientHelper::Get()->GetBrowsingHistory(
        max_count, days_ago, [=](const BrowsingHistoryList& browsing_history) {
          GetEligibleAds(user_model, dimensions, ad_events, browsing_history,
                         callback);
        });
  });
}

///////////////////////////////////////////////////////////////////////////////

void EligibleAdsV1::GetEligibleAds(
    const ad_targeting::UserModelInfo& user_model,
    const std::string& dimensions,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    GetEligibleAdsCallback callback) const {
  GetForParentChildSegments(user_model, dimensions, ad_events, browsing_history,
                            callback);
}

void EligibleAdsV1::GetForParentChildSegments(
    const ad_targeting::UserModelInfo& user_model,
    const std::string& dimensions,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    GetEligibleAdsCallback callback) const {
  const SegmentList segments =
      ad_targeting::GetTopParentChildSegments(user_model);
  if (segments.empty()) {
    GetForParentSegments(user_model, dimensions, ad_events, browsing_history,
                         callback);
    return;
  }

  BLOG(1, "Get eligible ads for parent-child segments:");
  for (const auto& segment : segments) {
    BLOG(1, "  " << segment);
  }

  database::table::CreativeInlineContentAds database_table;
  database_table.GetForSegmentsAndDimensions(
      segments, dimensions,
      [=](const bool success, const SegmentList& segments,
          const CreativeInlineContentAdList& creative_ads) {
        const CreativeInlineContentAdList eligible_creative_ads =
            FilterCreativeAds(creative_ads, ad_events, browsing_history);

        if (eligible_creative_ads.empty()) {
          BLOG(1, "No eligible ads for parent-child segments");
          GetForParentSegments(user_model, dimensions, ad_events,
                               browsing_history, callback);
          return;
        }

        callback(/* had_opportunity */ true, eligible_creative_ads);
      });
}

void EligibleAdsV1::GetForParentSegments(
    const ad_targeting::UserModelInfo& user_model,
    const std::string& dimensions,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    GetEligibleAdsCallback callback) const {
  const SegmentList segments = ad_targeting::GetTopParentSegments(user_model);
  if (segments.empty()) {
    GetForUntargeted(dimensions, ad_events, browsing_history, callback);
    return;
  }

  BLOG(1, "Get eligible ads for parent segments:");
  for (const auto& segment : segments) {
    BLOG(1, "  " << segment);
  }

  database::table::CreativeInlineContentAds database_table;
  database_table.GetForSegmentsAndDimensions(
      segments, dimensions,
      [=](const bool success, const SegmentList& segments,
          const CreativeInlineContentAdList& creative_ads) {
        CreativeInlineContentAdList eligible_creative_ads =
            FilterCreativeAds(creative_ads, ad_events, browsing_history);

        if (eligible_creative_ads.empty()) {
          BLOG(1, "No eligible ads for parent segments");
          GetForUntargeted(dimensions, ad_events, browsing_history, callback);
          return;
        }

        callback(/* had_opportunity */ true, eligible_creative_ads);
      });
}

void EligibleAdsV1::GetForUntargeted(
    const std::string& dimensions,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    GetEligibleAdsCallback callback) const {
  BLOG(1, "Get eligible ads for untargeted segment");

  database::table::CreativeInlineContentAds database_table;
  database_table.GetForSegmentsAndDimensions(
      {kUntargeted}, dimensions,
      [=](const bool success, const SegmentList& segments,
          const CreativeInlineContentAdList& creative_ads) {
        CreativeInlineContentAdList eligible_creative_ads =
            FilterCreativeAds(creative_ads, ad_events, browsing_history);

        if (eligible_creative_ads.empty()) {
          BLOG(1, "No eligible ads for untargeted segment");
        }

        callback(/* had_opportunity */ true, eligible_creative_ads);
      });
}

CreativeInlineContentAdList EligibleAdsV1::FilterCreativeAds(
    const CreativeInlineContentAdList& creative_ads,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history) const {
  if (creative_ads.empty()) {
    return {};
  }

  CreativeInlineContentAdList eligible_creative_ads = creative_ads;

  eligible_creative_ads = FilterSeenAdvertisersAndRoundRobinIfNeeded(
      eligible_creative_ads, AdType::kInlineContentAd);

  eligible_creative_ads = FilterSeenAdsAndRoundRobinIfNeeded(
      eligible_creative_ads, AdType::kInlineContentAd);

  eligible_creative_ads = ApplyFrequencyCapping(
      eligible_creative_ads,
      ShouldCapLastServedAd(creative_ads) ? last_served_ad_ : AdInfo(),
      ad_events, browsing_history);

  eligible_creative_ads = PaceAds(eligible_creative_ads);

  eligible_creative_ads = PrioritizeAds(eligible_creative_ads);

  return eligible_creative_ads;
}

CreativeInlineContentAdList EligibleAdsV1::ApplyFrequencyCapping(
    const CreativeInlineContentAdList& creative_ads,
    const AdInfo& last_served_ad,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history) const {
  CreativeInlineContentAdList eligible_creative_ads = creative_ads;

  const frequency_capping::ExclusionRules exclusion_rules(
      subdivision_targeting_, anti_targeting_resource_, ad_events,
      browsing_history);

  const auto iter = std::remove_if(
      eligible_creative_ads.begin(), eligible_creative_ads.end(),
      [&exclusion_rules, &last_served_ad](const CreativeAdInfo& creative_ad) {
        return exclusion_rules.ShouldExcludeCreativeAd(creative_ad) ||
               creative_ad.creative_instance_id ==
                   last_served_ad.creative_instance_id;
      });

  eligible_creative_ads.erase(iter, eligible_creative_ads.end());

  return eligible_creative_ads;
}

}  // namespace inline_content_ads
}  // namespace ads
