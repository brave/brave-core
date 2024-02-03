/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pipelines/inline_content_ads/eligible_inline_content_ads_v2.h"

#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/eligible_ads_feature.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rules_util.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/inline_content_ads/inline_content_ad_exclusion_rules.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pacing/pacing.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/priority/priority.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_ad_model_based_predictor.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"

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
      base::BindOnce(
          &EligibleInlineContentAdsV2::GetEligibleAdsForUserModelCallback,
          weak_factory_.GetWeakPtr(), std::move(user_model), dimensions,
          std::move(callback)));
}

///////////////////////////////////////////////////////////////////////////////

void EligibleInlineContentAdsV2::GetEligibleAdsForUserModelCallback(
    UserModelInfo user_model,
    const std::string& dimensions,
    EligibleAdsCallback<CreativeInlineContentAdList> callback,
    const bool success,
    const AdEventList& ad_events) {
  if (!success) {
    BLOG(1, "Failed to get ad events");
    return std::move(callback).Run(/*eligible_ads=*/{});
  }

  GetBrowsingHistory(
      kBrowsingHistoryMaxCount.Get(), kBrowsingHistoryRecentDayRange.Get(),
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
    return std::move(callback).Run(/*eligible_ads=*/{});
  }

  FilterAndMaybePredictCreativeAd(user_model, creative_ads, ad_events,
                                  browsing_history, std::move(callback));
}

void EligibleInlineContentAdsV2::FilterAndMaybePredictCreativeAd(
    const UserModelInfo& user_model,
    const CreativeInlineContentAdList& creative_ads,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    EligibleAdsCallback<CreativeInlineContentAdList> callback) {
  if (creative_ads.empty()) {
    BLOG(1, "No eligible ads");
    return std::move(callback).Run(/*eligible_ads=*/{});
  }

  CreativeInlineContentAdList eligible_creative_ads = creative_ads;
  FilterIneligibleCreativeAds(eligible_creative_ads, ad_events,
                              browsing_history);

  const PrioritizedCreativeAdBuckets<CreativeInlineContentAdList> buckets =
      SortCreativeAdsIntoBucketsByPriority(eligible_creative_ads);

  LogNumberOfCreativeAdsPerBucket(buckets);

  // For each bucket of prioritized ads attempt to predict the most suitable ad
  // for the user in priority order.
  for (const auto& [priority, prioritized_eligible_creative_ads] : buckets) {
    const std::optional<CreativeInlineContentAdInfo> predicted_creative_ad =
        MaybePredictCreativeAd(prioritized_eligible_creative_ads, user_model,
                               ad_events);
    if (!predicted_creative_ad) {
      // Could not predict an ad for this bucket, so continue to the next
      // bucket.
      continue;
    }

    BLOG(1, "Predicted ad with creative instance id "
                << predicted_creative_ad->creative_instance_id
                << " and a priority of " << priority);

    return std::move(callback).Run({*predicted_creative_ad});
  }

  // Could not predict an ad for any of the buckets.
  BLOG(1, "No eligible ads out of " << creative_ads.size() << " ads");
  std::move(callback).Run(/*eligible_ads=*/{});
}

void EligibleInlineContentAdsV2::FilterIneligibleCreativeAds(
    CreativeInlineContentAdList& creative_ads,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history) {
  if (creative_ads.empty()) {
    return;
  }

  InlineContentAdExclusionRules exclusion_rules(
      ad_events, *subdivision_targeting_, *anti_targeting_resource_,
      browsing_history);
  ApplyExclusionRules(creative_ads, last_served_ad_, &exclusion_rules);

  PaceCreativeAds(creative_ads);
}

}  // namespace brave_ads
