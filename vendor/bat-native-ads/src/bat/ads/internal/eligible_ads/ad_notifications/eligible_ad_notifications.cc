/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/ad_notifications/eligible_ad_notifications.h"

#include "base/check.h"
#include "bat/ads/ad_notification_info.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ad_pacing/ad_pacing.h"
#include "bat/ads/internal/ad_priority/ad_priority.h"
#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/ad_targeting/ad_targeting.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_user_model_info.h"
#include "bat/ads/internal/ads/ad_notifications/ad_notification_exclusion_rules.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/database/tables/ad_events_database_table.h"
#include "bat/ads/internal/database/tables/creative_ad_notifications_database_table.h"
#include "bat/ads/internal/eligible_ads/eligible_ads_constants.h"
#include "bat/ads/internal/eligible_ads/eligible_ads_predictor_util.h"
#include "bat/ads/internal/eligible_ads/eligible_ads_util.h"
#include "bat/ads/internal/eligible_ads/sample_ads.h"
#include "bat/ads/internal/eligible_ads/seen_ads.h"
#include "bat/ads/internal/eligible_ads/seen_advertisers.h"
#include "bat/ads/internal/features/ad_serving/ad_serving_features.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"
#include "bat/ads/internal/segments/segments_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace ad_notifications {

namespace {

bool ShouldCapLastServedAd(const CreativeAdNotificationList& ads) {
  return ads.size() != 1;
}

}  // namespace

EligibleAds::EligibleAds(
    ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
    resource::AntiTargeting* anti_targeting_resource)
    : subdivision_targeting_(subdivision_targeting),
      anti_targeting_resource_(anti_targeting_resource) {
  DCHECK(subdivision_targeting_);
  DCHECK(anti_targeting_resource_);
}

EligibleAds::~EligibleAds() = default;

void EligibleAds::SetLastServedAd(const CreativeAdInfo& creative_ad) {
  last_served_creative_ad_ = creative_ad;
}

void EligibleAds::Get(const ad_targeting::UserModelInfo& user_model,
                      GetEligibleAdsCallback callback) {
  database::table::AdEvents database_table;
  database_table.GetAll([=](const bool success, const AdEventList& ad_events) {
    if (!success) {
      BLOG(1, "Failed to get ad events");
      callback(/* was_allowed */ false, {});
      return;
    }

    const int max_count = features::GetBrowsingHistoryMaxCount();
    const int days_ago = features::GetBrowsingHistoryDaysAgo();
    AdsClientHelper::Get()->GetBrowsingHistory(
        max_count, days_ago, [=](const BrowsingHistoryList& browsing_history) {
          GetForParentChildSegments(user_model, ad_events, browsing_history,
                                    callback);
        });
  });
}

void EligibleAds::GetV2(const ad_targeting::UserModelInfo& user_model,
                        GetEligibleAdsV2Callback callback) {
  database::table::AdEvents database_table;
  database_table.GetAll([=](const bool success, const AdEventList& ad_events) {
    if (!success) {
      BLOG(1, "Failed to get ad events");
      callback(/* was_allowed */ false, absl::nullopt);
      return;
    }

    const int max_count = features::GetBrowsingHistoryMaxCount();
    const int days_ago = features::GetBrowsingHistoryDaysAgo();
    AdsClientHelper::Get()->GetBrowsingHistory(
        max_count, days_ago, [=](const BrowsingHistoryList& history) {
          GetEligibleAds(user_model, ad_events, history, callback);
        });
  });
}

///////////////////////////////////////////////////////////////////////////////

void EligibleAds::GetEligibleAds(const ad_targeting::UserModelInfo& user_model,
                                 const AdEventList& ad_events,
                                 const BrowsingHistoryList& browsing_history,
                                 GetEligibleAdsV2Callback callback) const {
  BLOG(1, "Get eligible ads");

  database::table::CreativeAdNotifications database_table;
  database_table.GetAll([=](const bool success, const SegmentList& segments,
                            const CreativeAdNotificationList& ads) {
    if (!success) {
      BLOG(1, "Failed to get ads");
      callback(/* was_allowed */ false, absl::nullopt);
      return;
    }

    const CreativeAdNotificationList eligible_ads = ApplyFrequencyCapping(
        ads,
        ShouldCapLastServedAd(ads) ? last_served_creative_ad_
                                   : CreativeAdInfo(),
        ad_events, browsing_history);

    if (eligible_ads.empty()) {
      BLOG(1, "No eligible ads");
      callback(/* was_allowed */ true, absl::nullopt);
      return;
    }

    ChooseAd(user_model, ad_events, eligible_ads, callback);
  });
}

void EligibleAds::ChooseAd(const ad_targeting::UserModelInfo& user_model,
                           const AdEventList& ad_events,
                           const CreativeAdNotificationList& eligible_ads,
                           GetEligibleAdsV2Callback callback) const {
  DCHECK(!eligible_ads.empty());

  const CreativeAdNotificationPredictorMap ads =
      GroupEligibleAdsByCreativeInstanceId(eligible_ads);

  const CreativeAdNotificationPredictorMap predictors =
      ComputePredictorFeaturesAndScores(ads, user_model, ad_events);

  const absl::optional<CreativeAdNotificationInfo> ad =
      SampleAdFromPredictors(predictors);

  callback(/* was_allowed */ true, ad);
}

void EligibleAds::GetForParentChildSegments(
    const ad_targeting::UserModelInfo& user_model,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    GetEligibleAdsCallback callback) const {
  const SegmentList segments =
      ad_targeting::GetTopParentChildSegments(user_model);
  if (segments.empty()) {
    GetForParentSegments(user_model, ad_events, browsing_history, callback);
    return;
  }

  BLOG(1, "Get eligible ads for parent-child segments:");
  for (const auto& segment : segments) {
    BLOG(1, "  " << segment);
  }

  database::table::CreativeAdNotifications database_table;
  database_table.GetForSegments(
      segments, [=](const bool success, const SegmentList& segments,
                    const CreativeAdNotificationList& ads) {
        CreativeAdNotificationList eligible_ads =
            FilterIneligibleAds(ads, ad_events, browsing_history);

        if (eligible_ads.empty()) {
          BLOG(1, "No eligible ads for parent-child segments");
          GetForParentSegments(user_model, ad_events, browsing_history,
                               callback);
          return;
        }

        callback(/* was_allowed */ true, eligible_ads);
      });
}

void EligibleAds::GetForParentSegments(
    const ad_targeting::UserModelInfo& user_model,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    GetEligibleAdsCallback callback) const {
  const SegmentList segments = ad_targeting::GetTopParentSegments(user_model);
  if (segments.empty()) {
    GetForUntargeted(ad_events, browsing_history, callback);
    return;
  }

  BLOG(1, "Get eligible ads for parent segments:");
  for (const auto& segment : segments) {
    BLOG(1, "  " << segment);
  }

  database::table::CreativeAdNotifications database_table;
  database_table.GetForSegments(
      segments, [=](const bool success, const SegmentList& segments,
                    const CreativeAdNotificationList& ads) {
        CreativeAdNotificationList eligible_ads =
            FilterIneligibleAds(ads, ad_events, browsing_history);

        if (eligible_ads.empty()) {
          BLOG(1, "No eligible ads for parent segments");
          GetForUntargeted(ad_events, browsing_history, callback);
          return;
        }

        callback(/* was_allowed */ true, eligible_ads);
      });
}

void EligibleAds::GetForUntargeted(const AdEventList& ad_events,
                                   const BrowsingHistoryList& browsing_history,
                                   GetEligibleAdsCallback callback) const {
  BLOG(1, "Get eligible ads for untargeted segment");

  database::table::CreativeAdNotifications database_table;
  database_table.GetForSegments(
      {kUntargeted}, [=](const bool success, const SegmentList& segments,
                         const CreativeAdNotificationList& ads) {
        CreativeAdNotificationList eligible_ads =
            FilterIneligibleAds(ads, ad_events, browsing_history);

        if (eligible_ads.empty()) {
          BLOG(1, "No eligible ads for untargeted segment");
        }

        callback(/* was_allowed */ true, eligible_ads);
      });
}

CreativeAdNotificationList EligibleAds::FilterIneligibleAds(
    const CreativeAdNotificationList& ads,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history) const {
  if (ads.empty()) {
    return {};
  }

  CreativeAdNotificationList eligible_ads = ads;

  eligible_ads = FilterSeenAdvertisersAndRoundRobinIfNeeded(
      eligible_ads, AdType::kAdNotification);

  eligible_ads =
      FilterSeenAdsAndRoundRobinIfNeeded(eligible_ads, AdType::kAdNotification);

  eligible_ads = ApplyFrequencyCapping(
      eligible_ads,
      ShouldCapLastServedAd(ads) ? last_served_creative_ad_ : CreativeAdInfo(),
      ad_events, browsing_history);

  eligible_ads = PaceAds(eligible_ads);

  eligible_ads = PrioritizeAds(eligible_ads);

  return eligible_ads;
}

CreativeAdNotificationList EligibleAds::ApplyFrequencyCapping(
    const CreativeAdNotificationList& ads,
    const CreativeAdInfo& last_served_creative_ad,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history) const {
  CreativeAdNotificationList eligible_ads = ads;

  const frequency_capping::ExclusionRules exclusion_rules(
      subdivision_targeting_, anti_targeting_resource_, ad_events,
      browsing_history);

  const auto iter = std::remove_if(
      eligible_ads.begin(), eligible_ads.end(),
      [&exclusion_rules, &last_served_creative_ad](CreativeAdInfo& ad) {
        return exclusion_rules.ShouldExcludeAd(ad) ||
               ad.creative_instance_id ==
                   last_served_creative_ad.creative_instance_id;
      });

  eligible_ads.erase(iter, eligible_ads.end());

  return eligible_ads;
}

}  // namespace ad_notifications
}  // namespace ads
