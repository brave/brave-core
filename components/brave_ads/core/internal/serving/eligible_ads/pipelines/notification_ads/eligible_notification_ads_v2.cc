/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pipelines/notification_ads/eligible_notification_ads_v2.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/eligible_ads_feature.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rules_util.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/notification_ads/notification_ad_exclusion_rules.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pacing/pacing.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_ad_model_based_predictor.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_events_database_table.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

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

  const database::table::AdEvents database_table;
  database_table.GetForType(
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
    BLOG(1, "Failed to get ad events");
    return std::move(callback).Run(/*eligible_ads=*/{});
  }

  GetBrowsingHistory(
      kBrowsingHistoryMaxCount.Get(), kBrowsingHistoryRecentDayRange.Get(),
      base::BindOnce(&EligibleNotificationAdsV2::GetEligibleAds,
                     weak_factory_.GetWeakPtr(), std::move(user_model),
                     ad_events, std::move(callback)));
}

void EligibleNotificationAdsV2::GetEligibleAds(
    UserModelInfo user_model,
    const AdEventList& ad_events,
    EligibleAdsCallback<CreativeNotificationAdList> callback,
    const BrowsingHistoryList& browsing_history) {
  const database::table::CreativeNotificationAds database_table;
  database_table.GetAll(
      base::BindOnce(&EligibleNotificationAdsV2::GetEligibleAdsCallback,
                     weak_factory_.GetWeakPtr(), std::move(user_model),
                     ad_events, browsing_history, std::move(callback)));
}

void EligibleNotificationAdsV2::GetEligibleAdsCallback(
    const UserModelInfo& user_model,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    EligibleAdsCallback<CreativeNotificationAdList> callback,
    const bool success,
    const SegmentList& /*segments=*/,
    const CreativeNotificationAdList& creative_ads) {
  if (!success) {
    BLOG(1, "Failed to get ads");
    return std::move(callback).Run(/*eligible_ads=*/{});
  }

  if (creative_ads.empty()) {
    BLOG(1, "No eligible ads");
    return std::move(callback).Run(/*eligible_ads=*/{});
  }

  const CreativeNotificationAdList eligible_creative_ads =
      FilterCreativeAds(creative_ads, ad_events, browsing_history);
  if (eligible_creative_ads.empty()) {
    BLOG(1, "No eligible ads out of " << creative_ads.size() << " ads");
    return std::move(callback).Run(/*eligible_ads=*/{});
  }

  const absl::optional<CreativeNotificationAdInfo> creative_ad =
      MaybePredictCreativeAd(eligible_creative_ads, user_model, ad_events);
  if (!creative_ad) {
    BLOG(1, "No eligible ads");
    return std::move(callback).Run(/*eligible_ads=*/{});
  }

  std::move(callback).Run({*creative_ad});
}

CreativeNotificationAdList EligibleNotificationAdsV2::FilterCreativeAds(
    const CreativeNotificationAdList& creative_ads,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history) {
  if (creative_ads.empty()) {
    return {};
  }

  CreativeNotificationAdList eligible_creative_ads = creative_ads;

  NotificationAdExclusionRules exclusion_rules(
      ad_events, *subdivision_targeting_, *anti_targeting_resource_,
      browsing_history);
  eligible_creative_ads = ApplyExclusionRules(
      eligible_creative_ads, last_served_ad_, &exclusion_rules);

  return PaceCreativeAds(eligible_creative_ads);
}

}  // namespace brave_ads
