/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/pipelines/new_tab_page_ads/eligible_new_tab_page_ads_v1.h"

#include <utility>

#include "base/functional/bind.h"
#include "bat/ads/internal/ads/ad_events/ad_events_database_table.h"
#include "bat/ads/internal/ads/serving/eligible_ads/allocation/seen_ads.h"
#include "bat/ads/internal/ads/serving/eligible_ads/allocation/seen_advertisers.h"
#include "bat/ads/internal/ads/serving/eligible_ads/eligible_ads_constants.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rules_util.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/new_tab_page_ads/new_tab_page_ad_exclusion_rules.h"
#include "bat/ads/internal/ads/serving/eligible_ads/pacing/pacing.h"
#include "bat/ads/internal/ads/serving/eligible_ads/priority/priority.h"
#include "bat/ads/internal/ads/serving/serving_features.h"
#include "bat/ads/internal/ads/serving/targeting/top_segments.h"
#include "bat/ads/internal/ads/serving/targeting/user_model_info.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_table.h"
#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"

namespace ads::new_tab_page_ads {

EligibleAdsV1::EligibleAdsV1(
    geographic::SubdivisionTargeting* subdivision_targeting,
    resource::AntiTargeting* anti_targeting_resource)
    : EligibleAdsBase(subdivision_targeting, anti_targeting_resource) {}

void EligibleAdsV1::GetForUserModel(
    targeting::UserModelInfo user_model,
    GetEligibleAdsCallback<CreativeNewTabPageAdList> callback) {
  BLOG(1, "Get eligible new tab page ads:");

  const database::table::AdEvents database_table;
  database_table.GetForType(
      mojom::AdType::kNewTabPageAd,
      base::BindOnce(&EligibleAdsV1::OnGetForUserModel, base::Unretained(this),
                     std::move(user_model), std::move(callback)));
}

///////////////////////////////////////////////////////////////////////////////

void EligibleAdsV1::OnGetForUserModel(
    targeting::UserModelInfo user_model,
    GetEligibleAdsCallback<CreativeNewTabPageAdList> callback,
    const bool success,
    const AdEventList& ad_events) {
  if (!success) {
    BLOG(1, "Failed to get ad events");
    std::move(callback).Run(/*had_opportunity*/ false, {});
    return;
  }

  GetBrowsingHistory(std::move(user_model), ad_events, std::move(callback));
}

void EligibleAdsV1::GetBrowsingHistory(
    targeting::UserModelInfo user_model,
    const AdEventList& ad_events,
    GetEligibleAdsCallback<CreativeNewTabPageAdList> callback) {
  const int max_count = features::GetBrowsingHistoryMaxCount();
  const int days_ago = features::GetBrowsingHistoryDaysAgo();
  AdsClientHelper::GetInstance()->GetBrowsingHistory(
      max_count, days_ago,
      base::BindOnce(&EligibleAdsV1::GetEligibleAds, base::Unretained(this),
                     std::move(user_model), ad_events, std::move(callback)));
}

void EligibleAdsV1::GetEligibleAds(
    targeting::UserModelInfo user_model,
    const AdEventList& ad_events,
    GetEligibleAdsCallback<CreativeNewTabPageAdList> callback,
    const BrowsingHistoryList& browsing_history) {
  GetForChildSegments(std::move(user_model), ad_events, browsing_history,
                      std::move(callback));
}

void EligibleAdsV1::GetForChildSegments(
    targeting::UserModelInfo user_model,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    GetEligibleAdsCallback<CreativeNewTabPageAdList> callback) {
  const SegmentList segments = targeting::GetTopChildSegments(user_model);
  if (segments.empty()) {
    GetForParentSegments(user_model, ad_events, browsing_history,
                         std::move(callback));
    return;
  }

  BLOG(1, "Get eligible ads for child segments:");
  for (const auto& segment : segments) {
    BLOG(1, "  " << segment);
  }

  const database::table::CreativeNewTabPageAds database_table;
  database_table.GetForSegments(
      segments,
      base::BindOnce(&EligibleAdsV1::OnGetForChildSegments,
                     base::Unretained(this), std::move(user_model), ad_events,
                     browsing_history, std::move(callback)));
}

void EligibleAdsV1::OnGetForChildSegments(
    const targeting::UserModelInfo& user_model,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    GetEligibleAdsCallback<CreativeNewTabPageAdList> callback,
    const bool success,
    const SegmentList& /*segments*/,
    const CreativeNewTabPageAdList& creative_ads) {
  if (!success) {
    BLOG(1, "Failed to get ads for child segments");
    std::move(callback).Run(/*had_opportunity*/ false, {});
    return;
  }

  const CreativeNewTabPageAdList eligible_creative_ads =
      FilterCreativeAds(creative_ads, ad_events, browsing_history);
  if (eligible_creative_ads.empty()) {
    BLOG(1, "No eligible ads out of " << creative_ads.size()
                                      << " ads for child segments");
    GetForParentSegments(user_model, ad_events, browsing_history,
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
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    GetEligibleAdsCallback<CreativeNewTabPageAdList> callback) {
  const SegmentList segments = targeting::GetTopParentSegments(user_model);
  if (segments.empty()) {
    GetForUntargeted(ad_events, browsing_history, std::move(callback));
    return;
  }

  BLOG(1, "Get eligible ads for parent segments:");
  for (const auto& segment : segments) {
    BLOG(1, "  " << segment);
  }

  const database::table::CreativeNewTabPageAds database_table;
  database_table.GetForSegments(
      segments, base::BindOnce(&EligibleAdsV1::OnGetForParentSegments,
                               base::Unretained(this), ad_events,
                               browsing_history, std::move(callback)));
}

void EligibleAdsV1::OnGetForParentSegments(
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    GetEligibleAdsCallback<CreativeNewTabPageAdList> callback,
    const bool success,
    const SegmentList& /*segments*/,
    const CreativeNewTabPageAdList& creative_ads) {
  if (!success) {
    BLOG(1, "Failed to get ads for parent segments");
    std::move(callback).Run(/*had_opportunity*/ false, {});
    return;
  }

  const CreativeNewTabPageAdList eligible_creative_ads =
      FilterCreativeAds(creative_ads, ad_events, browsing_history);
  if (eligible_creative_ads.empty()) {
    BLOG(1, "No eligible ads out of " << creative_ads.size()
                                      << " ads for parent segments");
    GetForUntargeted(ad_events, browsing_history, std::move(callback));
    return;
  }

  BLOG(1, eligible_creative_ads.size()
              << " eligible ads out of " << creative_ads.size()
              << " ads for parent segments");

  std::move(callback).Run(/*had_opportunity*/ true, eligible_creative_ads);
}

void EligibleAdsV1::GetForUntargeted(
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    GetEligibleAdsCallback<CreativeNewTabPageAdList> callback) {
  BLOG(1, "Get eligible ads for untargeted segment");

  const database::table::CreativeNewTabPageAds database_table;
  database_table.GetForSegments(
      {kUntargeted},
      base::BindOnce(&EligibleAdsV1::OnGetForUntargeted, base::Unretained(this),
                     ad_events, browsing_history, std::move(callback)));
}

void EligibleAdsV1::OnGetForUntargeted(
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    GetEligibleAdsCallback<CreativeNewTabPageAdList> callback,
    const bool success,
    const SegmentList& /*segments*/,
    const CreativeNewTabPageAdList& creative_ads) {
  if (!success) {
    BLOG(1, "Failed to get ads for untargeted segment");
    std::move(callback).Run(/*had_opportunity*/ false, {});
    return;
  }

  const CreativeNewTabPageAdList eligible_creative_ads =
      FilterCreativeAds(creative_ads, ad_events, browsing_history);
  if (eligible_creative_ads.empty()) {
    BLOG(1, "No eligible ads out of " << creative_ads.size()
                                      << " ads for untargeted segment");
    std::move(callback).Run(/*had_opportunity*/ false, {});
    return;
  }

  std::move(callback).Run(/*had_opportunity*/ true, eligible_creative_ads);
}

CreativeNewTabPageAdList EligibleAdsV1::FilterCreativeAds(
    const CreativeNewTabPageAdList& creative_ads,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history) {
  if (creative_ads.empty()) {
    return {};
  }

  CreativeNewTabPageAdList eligible_creative_ads = creative_ads;

  ExclusionRules exclusion_rules(ad_events, subdivision_targeting_,
                                 anti_targeting_resource_, browsing_history);
  eligible_creative_ads = ApplyExclusionRules(
      eligible_creative_ads, last_served_ad_, &exclusion_rules);

  eligible_creative_ads = FilterSeenAdvertisersAndRoundRobinIfNeeded(
      eligible_creative_ads, AdType::kNewTabPageAd);

  eligible_creative_ads = FilterSeenAdsAndRoundRobinIfNeeded(
      eligible_creative_ads, AdType::kNewTabPageAd);

  eligible_creative_ads = PaceCreativeAds(eligible_creative_ads);

  eligible_creative_ads = PrioritizeCreativeAds(eligible_creative_ads);

  return eligible_creative_ads;
}

}  // namespace ads::new_tab_page_ads
