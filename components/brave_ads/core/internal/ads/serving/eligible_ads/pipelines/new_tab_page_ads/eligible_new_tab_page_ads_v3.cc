/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/pipelines/new_tab_page_ads/eligible_new_tab_page_ads_v3.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/internal/ads/serving/choose/predict_ad.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rules_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/new_tab_page_ads/new_tab_page_ad_exclusion_rules.h"
#include "brave/components/brave_ads/core/internal/ads/serving/serving_features.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_info.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads::new_tab_page_ads {

EligibleAdsV3::EligibleAdsV3(
    geographic::SubdivisionTargeting* subdivision_targeting,
    resource::AntiTargeting* anti_targeting_resource)
    : EligibleAdsBase(subdivision_targeting, anti_targeting_resource) {}

void EligibleAdsV3::GetForUserModel(
    targeting::UserModelInfo user_model,
    GetEligibleAdsCallback<CreativeNewTabPageAdList> callback) {
  BLOG(1, "Get eligible new tab page ads");

  database::table::AdEvents database_table;
  database_table.GetForType(
      mojom::AdType::kNewTabPageAd,
      base::BindOnce(&EligibleAdsV3::OnGetForUserModel, base::Unretained(this),
                     std::move(user_model), std::move(callback)));
}

///////////////////////////////////////////////////////////////////////////////

void EligibleAdsV3::OnGetForUserModel(
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

void EligibleAdsV3::GetBrowsingHistory(
    targeting::UserModelInfo user_model,
    const AdEventList& ad_events,
    GetEligibleAdsCallback<CreativeNewTabPageAdList> callback) {
  const int max_count = features::GetBrowsingHistoryMaxCount();
  const int days_ago = features::GetBrowsingHistoryDaysAgo();
  AdsClientHelper::GetInstance()->GetBrowsingHistory(
      max_count, days_ago,
      base::BindOnce(&EligibleAdsV3::GetEligibleAds, base::Unretained(this),
                     std::move(user_model), ad_events, std::move(callback)));
}

void EligibleAdsV3::GetEligibleAds(
    targeting::UserModelInfo user_model,
    const AdEventList& ad_events,
    GetEligibleAdsCallback<CreativeNewTabPageAdList> callback,
    const BrowsingHistoryList& browsing_history) {
  database::table::CreativeNewTabPageAds database_table;
  database_table.GetAll(base::BindOnce(
      &EligibleAdsV3::OnGetEligibleAds, base::Unretained(this), user_model,
      ad_events, browsing_history, std::move(callback)));
}

void EligibleAdsV3::OnGetEligibleAds(
    const targeting::UserModelInfo& user_model,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    GetEligibleAdsCallback<CreativeNewTabPageAdList> callback,
    const bool success,
    const SegmentList& /*segments*/,
    const CreativeNewTabPageAdList& creative_ads) {
  if (!success) {
    BLOG(1, "Failed to get ads");
    std::move(callback).Run(/*had_opportunity*/ false, {});
    return;
  }

  if (creative_ads.empty()) {
    BLOG(1, "No eligible ads");
    std::move(callback).Run(/*had_opportunity*/ false, {});
    return;
  }

  const CreativeNewTabPageAdList eligible_creative_ads =
      FilterCreativeAds(creative_ads, ad_events, browsing_history);
  if (eligible_creative_ads.empty()) {
    BLOG(1, "No eligible ads out of " << creative_ads.size() << " ads");
    std::move(callback).Run(/*had_opportunity*/ true, {});
    return;
  }

  const absl::optional<CreativeNewTabPageAdInfo> creative_ad =
      PredictAd(user_model, ad_events, eligible_creative_ads);
  if (!creative_ad) {
    BLOG(1, "No eligible ads out of " << creative_ads.size() << " ads");
    std::move(callback).Run(/*had_opportunity*/ true, {});
    return;
  }

  BLOG(1, eligible_creative_ads.size()
              << " eligible ads out of " << creative_ads.size() << " ads");

  std::move(callback).Run(/*had_opportunity*/ true, {*creative_ad});
}

CreativeNewTabPageAdList EligibleAdsV3::FilterCreativeAds(
    const CreativeNewTabPageAdList& creative_ads,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history) {
  if (creative_ads.empty()) {
    return {};
  }

  ExclusionRules exclusion_rules(ad_events, subdivision_targeting_,
                                 anti_targeting_resource_, browsing_history);
  return ApplyExclusionRules(creative_ads, last_served_ad_, &exclusion_rules);
}

}  // namespace brave_ads::new_tab_page_ads
