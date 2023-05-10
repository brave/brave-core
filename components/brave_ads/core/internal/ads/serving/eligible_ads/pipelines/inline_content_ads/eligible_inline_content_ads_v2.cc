/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/pipelines/inline_content_ads/eligible_inline_content_ads_v2.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/internal/ads/serving/choose/predict_ad.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/eligible_ads_feature.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rules_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/inline_content_ads/inline_content_ad_exclusion_rules.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_info.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

EligibleInlineContentAdsV2::EligibleInlineContentAdsV2(
    const SubdivisionTargeting& subdivision_targeting,
    const AntiTargetingResource& anti_targeting_resource)
    : EligibleInlineContentAdsBase(subdivision_targeting,
                                   anti_targeting_resource) {}

EligibleInlineContentAdsV2::~EligibleInlineContentAdsV2() = default;

void EligibleInlineContentAdsV2::GetForUserModel(
    UserModelInfo user_model,
    const std::string& dimensions,
    EligibleAdsCallback<CreativeInlineContentAdList> callback) {
  BLOG(1, "Get eligible inline content ads");

  const database::table::AdEvents database_table;
  database_table.GetForType(
      mojom::AdType::kInlineContentAd,
      base::BindOnce(&EligibleInlineContentAdsV2::GetForUserModelCallback,
                     weak_factory_.GetWeakPtr(), std::move(user_model),
                     dimensions, std::move(callback)));
}

///////////////////////////////////////////////////////////////////////////////

void EligibleInlineContentAdsV2::GetForUserModelCallback(
    UserModelInfo user_model,
    const std::string& dimensions,
    EligibleAdsCallback<CreativeInlineContentAdList> callback,
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

void EligibleInlineContentAdsV2::GetBrowsingHistory(
    UserModelInfo user_model,
    const AdEventList& ad_events,
    const std::string& dimensions,
    EligibleAdsCallback<CreativeInlineContentAdList> callback) {
  AdsClientHelper::GetInstance()->GetBrowsingHistory(
      kBrowsingHistoryMaxCount.Get(), kBrowsingHistoryDaysAgo.Get(),
      base::BindOnce(&EligibleInlineContentAdsV2::GetEligibleAds,
                     weak_factory_.GetWeakPtr(), std::move(user_model),
                     ad_events, dimensions, std::move(callback)));
}

void EligibleInlineContentAdsV2::GetEligibleAds(
    UserModelInfo user_model,
    const AdEventList& ad_events,
    const std::string& dimensions,
    EligibleAdsCallback<CreativeInlineContentAdList> callback,
    const BrowsingHistoryList& browsing_history) {
  const database::table::CreativeInlineContentAds database_table;
  database_table.GetForDimensions(
      dimensions,
      base::BindOnce(&EligibleInlineContentAdsV2::GetEligibleAdsCallback,
                     weak_factory_.GetWeakPtr(), std::move(user_model),
                     ad_events, browsing_history, std::move(callback)));
}

void EligibleInlineContentAdsV2::GetEligibleAdsCallback(
    const UserModelInfo& user_model,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    EligibleAdsCallback<CreativeInlineContentAdList> callback,
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

CreativeInlineContentAdList EligibleInlineContentAdsV2::FilterCreativeAds(
    const CreativeInlineContentAdList& creative_ads,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history) {
  if (creative_ads.empty()) {
    return {};
  }

  InlineContentAdExclusionRules exclusion_rules(
      ad_events, *subdivision_targeting_, *anti_targeting_resource_,
      browsing_history);
  return ApplyExclusionRules(creative_ads, last_served_ad_, &exclusion_rules);
}

}  // namespace brave_ads
