/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/pipelines/inline_content_ads/eligible_inline_content_ads_v2.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom-shared.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/internal/ads/serving/choose/predict_ad.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/eligible_ads_features.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rules_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/inline_content_ads/inline_content_ad_exclusion_rules.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_info.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads::inline_content_ads {

EligibleAdsV2::EligibleAdsV2(
    const geographic::SubdivisionTargeting& subdivision_targeting,
    const resource::AntiTargeting& anti_targeting_resource)
    : EligibleAdsBase(subdivision_targeting, anti_targeting_resource) {}

EligibleAdsV2::~EligibleAdsV2() = default;

void EligibleAdsV2::GetForUserModel(
    targeting::UserModelInfo user_model,
    const std::string& dimensions,
    GetEligibleAdsCallback<CreativeInlineContentAdList> callback) {
  BLOG(1, "Get eligible inline content ads");

  const database::table::AdEvents database_table;
  database_table.GetForType(
      mojom::AdType::kInlineContentAd,
      base::BindOnce(&EligibleAdsV2::OnGetForUserModel,
                     weak_factory_.GetWeakPtr(), std::move(user_model),
                     dimensions, std::move(callback)));
}

///////////////////////////////////////////////////////////////////////////////

void EligibleAdsV2::OnGetForUserModel(
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

  GetBrowsingHistory(std::move(user_model), ad_events, dimensions,
                     std::move(callback));
}

void EligibleAdsV2::GetBrowsingHistory(
    targeting::UserModelInfo user_model,
    const AdEventList& ad_events,
    const std::string& dimensions,
    GetEligibleAdsCallback<CreativeInlineContentAdList> callback) {
  AdsClientHelper::GetInstance()->GetBrowsingHistory(
      kBrowsingHistoryMaxCount.Get(), kBrowsingHistoryDaysAgo.Get(),
      base::BindOnce(&EligibleAdsV2::GetEligibleAds, weak_factory_.GetWeakPtr(),
                     std::move(user_model), ad_events, dimensions,
                     std::move(callback)));
}

void EligibleAdsV2::GetEligibleAds(
    targeting::UserModelInfo user_model,
    const AdEventList& ad_events,
    const std::string& dimensions,
    GetEligibleAdsCallback<CreativeInlineContentAdList> callback,
    const BrowsingHistoryList& browsing_history) {
  const database::table::CreativeInlineContentAds database_table;
  database_table.GetForDimensions(
      dimensions,
      base::BindOnce(&EligibleAdsV2::OnGetEligibleAds,
                     weak_factory_.GetWeakPtr(), std::move(user_model),
                     ad_events, browsing_history, std::move(callback)));
}

void EligibleAdsV2::OnGetEligibleAds(
    const targeting::UserModelInfo& user_model,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    GetEligibleAdsCallback<CreativeInlineContentAdList> callback,
    const bool success,
    const CreativeInlineContentAdList& creative_ads) {
  if (!success) {
    BLOG(1, "Failed to get ads");
    return std::move(callback).Run(/*had_opportunity*/ false,
                                   /*eligible_ads*/ {});
  }

  if (creative_ads.empty()) {
    BLOG(1, "No eligible ads");
    return std::move(callback).Run(/*had_opportunity*/ false,
                                   /*eligible_ads*/ {});
  }

  const CreativeInlineContentAdList eligible_creative_ads =
      FilterCreativeAds(creative_ads, ad_events, browsing_history);
  if (eligible_creative_ads.empty()) {
    BLOG(1, "No eligible ads out of " << creative_ads.size() << " ads");
    return std::move(callback).Run(/*had_opportunity*/ true,
                                   /*eligible_ads*/ {});
  }

  const absl::optional<CreativeInlineContentAdInfo> creative_ad =
      PredictAd(user_model, ad_events, eligible_creative_ads);
  if (!creative_ad) {
    BLOG(1, "No eligible ads out of " << creative_ads.size() << " ads");
    return std::move(callback).Run(/*had_opportunity*/ true,
                                   /*eligible_ads*/ {});
  }

  BLOG(1, eligible_creative_ads.size()
              << " eligible ads out of " << creative_ads.size() << " ads");

  std::move(callback).Run(/*had_opportunity*/ true, {*creative_ad});
}

CreativeInlineContentAdList EligibleAdsV2::FilterCreativeAds(
    const CreativeInlineContentAdList& creative_ads,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history) {
  if (creative_ads.empty()) {
    return {};
  }

  ExclusionRules exclusion_rules(ad_events, *subdivision_targeting_,
                                 *anti_targeting_resource_, browsing_history);
  return ApplyExclusionRules(creative_ads, last_served_ad_, &exclusion_rules);
}

}  // namespace brave_ads::inline_content_ads
