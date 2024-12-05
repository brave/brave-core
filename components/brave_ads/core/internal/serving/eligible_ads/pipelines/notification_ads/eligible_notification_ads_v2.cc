/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pipelines/notification_ads/eligible_notification_ads_v2.h"

#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/eligible_ads_feature.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rules_util.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/notification_ads/notification_ad_exclusion_rules.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pacing/pacing.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/priority/priority.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_ad_model_based_predictor.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"

namespace brave_ads {

EligibleNotificationAdsV2::EligibleNotificationAdsV2(
    const SubdivisionTargeting& subdivision_targeting,
    const AntiTargetingResource& anti_targeting_resource)
    : EligibleNotificationAdsBase(subdivision_targeting,
                                  anti_targeting_resource) {}

EligibleNotificationAdsV2::~EligibleNotificationAdsV2() = default;

void EligibleNotificationAdsV2::GetForUserModel(
    UserModelInfo user_model,
    EligibleAdsCallback<CreativeNotificationAdList> callback) {
  BLOG(1, "Get eligible notification ads");

  ad_events_database_table_.GetUnexpired(
      mojom::AdType::kNotificationAd,
      base::BindOnce(
          &EligibleNotificationAdsV2::GetEligibleAdsForUserModelCallback,
          weak_factory_.GetWeakPtr(), std::move(user_model),
          std::move(callback)));
}

///////////////////////////////////////////////////////////////////////////////

void EligibleNotificationAdsV2::GetEligibleAdsForUserModelCallback(
    UserModelInfo user_model,
    EligibleAdsCallback<CreativeNotificationAdList> callback,
    const bool success,
    const AdEventList& ad_events) {
  if (!success) {
    BLOG(0, "Failed to get ad events");
    return std::move(callback).Run(/*eligible_ads=*/{});
  }

  GetAdsClient().GetSiteHistory(
      kSiteHistoryMaxCount.Get(), kSiteHistoryRecentDayRange.Get(),
      base::BindOnce(&EligibleNotificationAdsV2::GetEligibleAds,
                     weak_factory_.GetWeakPtr(), std::move(user_model),
                     ad_events, std::move(callback)));
}

void EligibleNotificationAdsV2::GetEligibleAds(
    UserModelInfo user_model,
    const AdEventList& ad_events,
    EligibleAdsCallback<CreativeNotificationAdList> callback,
    const SiteHistoryList& site_history) {
  creative_ads_database_table_.GetForActiveCampaigns(
      base::BindOnce(&EligibleNotificationAdsV2::GetEligibleAdsCallback,
                     weak_factory_.GetWeakPtr(), std::move(user_model),
                     ad_events, site_history, std::move(callback)));
}

void EligibleNotificationAdsV2::GetEligibleAdsCallback(
    const UserModelInfo& user_model,
    const AdEventList& ad_events,
    const SiteHistoryList& site_history,
    EligibleAdsCallback<CreativeNotificationAdList> callback,
    const bool success,
    const SegmentList& /*segments*/,
    const CreativeNotificationAdList& creative_ads) {
  if (!success) {
    BLOG(0, "Failed to get ads");
    return std::move(callback).Run(/*eligible_ads=*/{});
  }

  FilterAndMaybePredictCreativeAd(user_model, creative_ads, ad_events,
                                  site_history, std::move(callback));
}

void EligibleNotificationAdsV2::FilterAndMaybePredictCreativeAd(
    const UserModelInfo& user_model,
    const CreativeNotificationAdList& creative_ads,
    const AdEventList& ad_events,
    const SiteHistoryList& site_history,
    EligibleAdsCallback<CreativeNotificationAdList> callback) {
  if (creative_ads.empty()) {
    BLOG(1, "No eligible ads");
    return std::move(callback).Run(/*eligible_ads=*/{});
  }

  CreativeNotificationAdList eligible_creative_ads = creative_ads;
  FilterIneligibleCreativeAds(eligible_creative_ads, ad_events, site_history);

  const PrioritizedCreativeAdBuckets<CreativeNotificationAdList> buckets =
      SortCreativeAdsIntoBucketsByPriority(eligible_creative_ads);

  LogNumberOfCreativeAdsPerBucket(buckets);

  // For each bucket of prioritized ads attempt to predict the most suitable ad
  // for the user in priority order.
  for (const auto& [priority, prioritized_eligible_creative_ads] : buckets) {
    const std::optional<CreativeNotificationAdInfo> predicted_creative_ad =
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

void EligibleNotificationAdsV2::FilterIneligibleCreativeAds(
    CreativeNotificationAdList& creative_ads,
    const AdEventList& ad_events,
    const SiteHistoryList& site_history) {
  if (creative_ads.empty()) {
    return;
  }

  NotificationAdExclusionRules exclusion_rules(
      ad_events, *subdivision_targeting_, *anti_targeting_resource_,
      site_history);
  ApplyExclusionRules(creative_ads, last_served_ad_, &exclusion_rules);

  PaceCreativeAds(creative_ads);
}

}  // namespace brave_ads
