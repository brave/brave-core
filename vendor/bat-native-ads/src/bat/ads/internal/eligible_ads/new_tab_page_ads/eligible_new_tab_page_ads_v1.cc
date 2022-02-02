/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/new_tab_page_ads/eligible_new_tab_page_ads_v1.h"

#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ad_pacing/ad_pacing.h"
#include "bat/ads/internal/ad_priority/ad_priority.h"
#include "bat/ads/internal/ad_serving/ad_serving_features.h"
#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/ad_targeting/ad_targeting.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_user_model_info.h"
#include "bat/ads/internal/ads/new_tab_page_ads/new_tab_page_ad_exclusion_rules.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/database/tables/ad_events_database_table.h"
#include "bat/ads/internal/database/tables/creative_new_tab_page_ads_database_table.h"
#include "bat/ads/internal/eligible_ads/eligible_ads_constants.h"
#include "bat/ads/internal/eligible_ads/frequency_capping.h"
#include "bat/ads/internal/eligible_ads/seen_ads.h"
#include "bat/ads/internal/eligible_ads/seen_advertisers.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting/anti_targeting_resource.h"

namespace ads {
namespace new_tab_page_ads {

EligibleAdsV1::EligibleAdsV1(
    ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
    resource::AntiTargeting* anti_targeting_resource)
    : EligibleAdsBase(subdivision_targeting, anti_targeting_resource) {}

EligibleAdsV1::~EligibleAdsV1() = default;

void EligibleAdsV1::GetForUserModel(
    const ad_targeting::UserModelInfo& user_model,
    GetEligibleAdsCallback<CreativeNewTabPageAdList> callback) {
  BLOG(1, "Get eligible new tab page ads:");

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
          GetEligibleAds(user_model, ad_events, browsing_history, callback);
        });
  });
}

///////////////////////////////////////////////////////////////////////////////

void EligibleAdsV1::GetEligibleAds(
    const ad_targeting::UserModelInfo& user_model,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    GetEligibleAdsCallback<CreativeNewTabPageAdList> callback) {
  GetForParentChildSegments(user_model, ad_events, browsing_history, callback);
}

void EligibleAdsV1::GetForParentChildSegments(
    const ad_targeting::UserModelInfo& user_model,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    GetEligibleAdsCallback<CreativeNewTabPageAdList> callback) {
  const SegmentList& segments =
      ad_targeting::GetTopParentChildSegments(user_model);
  if (segments.empty()) {
    GetForParentSegments(user_model, ad_events, browsing_history, callback);
    return;
  }

  BLOG(1, "Get eligible ads for parent-child segments:");
  for (const auto& segment : segments) {
    BLOG(1, "  " << segment);
  }

  database::table::CreativeNewTabPageAds database_table;
  database_table.GetForSegments(
      segments, [=](const bool success, const SegmentList& segments,
                    const CreativeNewTabPageAdList& creative_ads) {
        if (!success) {
          BLOG(1, "Failed to get ads for parent-child segments");
          callback(/* had_opportunity */ false, {});
          return;
        }

        const CreativeNewTabPageAdList& eligible_creative_ads =
            FilterCreativeAds(creative_ads, ad_events, browsing_history);
        if (eligible_creative_ads.empty()) {
          BLOG(1, "No eligible ads for parent-child segments");
          GetForParentSegments(user_model, ad_events, browsing_history,
                               callback);
          return;
        }

        callback(/* had_opportunity */ true, eligible_creative_ads);
      });
}

void EligibleAdsV1::GetForParentSegments(
    const ad_targeting::UserModelInfo& user_model,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    GetEligibleAdsCallback<CreativeNewTabPageAdList> callback) {
  const SegmentList& segments = ad_targeting::GetTopParentSegments(user_model);
  if (segments.empty()) {
    GetForUntargeted(ad_events, browsing_history, callback);
    return;
  }

  BLOG(1, "Get eligible ads for parent segments:");
  for (const auto& segment : segments) {
    BLOG(1, "  " << segment);
  }

  database::table::CreativeNewTabPageAds database_table;
  database_table.GetForSegments(
      segments, [=](const bool success, const SegmentList& segments,
                    const CreativeNewTabPageAdList& creative_ads) {
        if (!success) {
          BLOG(1, "Failed to get ads for parent segments");
          callback(/* had_opportunity */ false, {});
          return;
        }

        const CreativeNewTabPageAdList& eligible_creative_ads =
            FilterCreativeAds(creative_ads, ad_events, browsing_history);
        if (eligible_creative_ads.empty()) {
          BLOG(1, "No eligible ads for parent segments");
          GetForUntargeted(ad_events, browsing_history, callback);
          return;
        }

        callback(/* had_opportunity */ true, eligible_creative_ads);
      });
}

void EligibleAdsV1::GetForUntargeted(
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    GetEligibleAdsCallback<CreativeNewTabPageAdList> callback) {
  BLOG(1, "Get eligible ads for untargeted segment");

  database::table::CreativeNewTabPageAds database_table;
  database_table.GetForSegments(
      {kUntargeted}, [=](const bool success, const SegmentList& segments,
                         const CreativeNewTabPageAdList& creative_ads) {
        if (!success) {
          BLOG(1, "Failed to get ads for untargeted segment");
          callback(/* had_opportunity */ false, {});
          return;
        }

        const CreativeNewTabPageAdList& eligible_creative_ads =
            FilterCreativeAds(creative_ads, ad_events, browsing_history);
        if (eligible_creative_ads.empty()) {
          BLOG(1, "No eligible ads for untargeted segment");
        }

        callback(/* had_opportunity */ true, eligible_creative_ads);
      });
}

CreativeNewTabPageAdList EligibleAdsV1::FilterCreativeAds(
    const CreativeNewTabPageAdList& creative_ads,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history) {
  if (creative_ads.empty()) {
    return {};
  }

  CreativeNewTabPageAdList eligible_creative_ads = creative_ads;

  frequency_capping::ExclusionRules exclusion_rules(
      ad_events, subdivision_targeting_, anti_targeting_resource_,
      browsing_history);
  eligible_creative_ads = ApplyFrequencyCapping(
      eligible_creative_ads, last_served_ad_, &exclusion_rules);

  eligible_creative_ads = FilterSeenAdvertisersAndRoundRobinIfNeeded(
      eligible_creative_ads, AdType::kNewTabPageAd);

  eligible_creative_ads = FilterSeenAdsAndRoundRobinIfNeeded(
      eligible_creative_ads, AdType::kNewTabPageAd);

  eligible_creative_ads = PaceAds(eligible_creative_ads);

  eligible_creative_ads = PrioritizeAds(eligible_creative_ads);

  return eligible_creative_ads;
}

}  // namespace new_tab_page_ads
}  // namespace ads
