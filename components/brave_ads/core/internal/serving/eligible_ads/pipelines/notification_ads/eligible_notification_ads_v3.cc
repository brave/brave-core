/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pipelines/notification_ads/eligible_notification_ads_v3.h"

#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/eligible_ads_feature.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rules_util.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/notification_ads/notification_ad_exclusion_rules.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pacing/pacing.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/embedding_based/creative_ad_embedding_based_predictor.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_ad_model_based_predictor.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"

namespace brave_ads {

EligibleNotificationAdsV3::EligibleNotificationAdsV3(
    const SubdivisionTargeting& subdivision_targeting,
    const AntiTargetingResource& anti_targeting_resource)
    : EligibleNotificationAdsBase(subdivision_targeting,
                                  anti_targeting_resource) {}

EligibleNotificationAdsV3::~EligibleNotificationAdsV3() = default;

void EligibleNotificationAdsV3::GetForUserModel(
    UserModelInfo user_model,
    EligibleAdsCallback<CreativeNotificationAdList> callback) {
  BLOG(1, "Get eligible notification ads");

  database::table::AdEvents database_table;
  database_table.GetForType(
      mojom::AdType::kNotificationAd,
      base::BindOnce(
          &EligibleNotificationAdsV3::GetEligibleAdsForUserModelCallback,
          weak_factory_.GetWeakPtr(), std::move(user_model),
          std::move(callback)));
}

///////////////////////////////////////////////////////////////////////////////

void EligibleNotificationAdsV3::GetEligibleAdsForUserModelCallback(
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
      base::BindOnce(&EligibleNotificationAdsV3::GetEligibleAds,
                     weak_factory_.GetWeakPtr(), std::move(user_model),
                     ad_events, std::move(callback)));
}

void EligibleNotificationAdsV3::GetEligibleAds(
    UserModelInfo user_model,
    const AdEventList& ad_events,
    EligibleAdsCallback<CreativeNotificationAdList> callback,
    const BrowsingHistoryList& browsing_history) {
  const database::table::CreativeNotificationAds database_table;
  database_table.GetAll(
      base::BindOnce(&EligibleNotificationAdsV3::GetEligibleAdsCallback,
                     weak_factory_.GetWeakPtr(), user_model, ad_events,
                     browsing_history, std::move(callback)));
}

void EligibleNotificationAdsV3::GetEligibleAdsCallback(
    const UserModelInfo& user_model,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    EligibleAdsCallback<CreativeNotificationAdList> callback,
    const bool success,
    const SegmentList& /*segments*/,
    const CreativeNotificationAdList& creative_ads) {
  if (!success) {
    BLOG(1, "Failed to get ads");
    return std::move(callback).Run(/*eligible_ads=*/{});
  }

  if (creative_ads.empty()) {
    BLOG(1, "No eligible ads");
    return std::move(callback).Run(/*eligible_ads=*/{});
  }

  CreativeNotificationAdList eligible_creative_ads = creative_ads;
  FilterIneligibleCreativeAds(eligible_creative_ads, ad_events,
                              browsing_history);
  if (eligible_creative_ads.empty()) {
    BLOG(1, "No eligible ads out of " << creative_ads.size() << " ads");
    return std::move(callback).Run(/*eligible_ads=*/{});
  }

  const std::optional<CreativeNotificationAdInfo> creative_ad =
      user_model.interest.text_embedding_html_events.empty()
          ? MaybePredictCreativeAd(eligible_creative_ads, user_model, ad_events)
          : MaybePredictCreativeAd(eligible_creative_ads, user_model);
  if (!creative_ad) {
    BLOG(1, "No eligible ads");
    return std::move(callback).Run(/*eligible_ads=*/{});
  }

  std::move(callback).Run({*creative_ad});
}

void EligibleNotificationAdsV3::FilterIneligibleCreativeAds(
    CreativeNotificationAdList& creative_ads,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history) {
  if (creative_ads.empty()) {
    return;
  }

  NotificationAdExclusionRules exclusion_rules(
      ad_events, *subdivision_targeting_, *anti_targeting_resource_,
      browsing_history);
  ApplyExclusionRules(creative_ads, last_served_ad_, &exclusion_rules);

  PaceCreativeAds(creative_ads);
}

}  // namespace brave_ads
