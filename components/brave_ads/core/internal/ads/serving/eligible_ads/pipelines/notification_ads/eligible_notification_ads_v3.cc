/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/pipelines/notification_ads/eligible_notification_ads_v3.h"

#include "absl/types/optional.h"
#include "base/bind.h"
#include "bat/ads/internal/ads/ad_events/ad_events_database_table.h"
#include "bat/ads/internal/ads/serving/choose/predict_ad_embeddings.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rules_util.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/notification_ads/notification_ad_exclusion_rules.h"
#include "bat/ads/internal/ads/serving/serving_features.h"
#include "bat/ads/internal/ads/serving/targeting/user_model_info.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ads_database_table.h"
#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "bat/ads/internal/segments/segment_alias.h"

namespace ads::notification_ads {

EligibleAdsV3::EligibleAdsV3(
    geographic::SubdivisionTargeting* subdivision_targeting,
    resource::AntiTargeting* anti_targeting_resource)
    : EligibleAdsBase(subdivision_targeting, anti_targeting_resource) {}

void EligibleAdsV3::GetForUserModel(
    const targeting::UserModelInfo& user_model,
    GetEligibleAdsCallback<CreativeNotificationAdList> callback) {
  BLOG(1, "Get eligible notification ads");

  database::table::AdEvents database_table;
  database_table.GetForType(
      mojom::AdType::kNotificationAd,
      [=](const bool success, const AdEventList& ad_events) {
        if (!success) {
          BLOG(1, "Failed to get ad events");
          callback(/*had_opportunity*/ false, {});
          return;
        }

        GetBrowsingHistory(user_model, ad_events, callback);
      });
}

///////////////////////////////////////////////////////////////////////////////

void EligibleAdsV3::GetBrowsingHistory(
    const targeting::UserModelInfo& user_model,
    const AdEventList& ad_events,
    GetEligibleAdsCallback<CreativeNotificationAdList> callback) {
  const int max_count = features::GetBrowsingHistoryMaxCount();
  const int days_ago = features::GetBrowsingHistoryDaysAgo();
  AdsClientHelper::GetInstance()->GetBrowsingHistory(
      max_count, days_ago,
      base::BindOnce(&EligibleAdsV3::GetEligibleAds, base::Unretained(this),
                     user_model, ad_events, callback));
}

void EligibleAdsV3::GetEligibleAds(
    const targeting::UserModelInfo& user_model,
    const AdEventList& ad_events,
    GetEligibleAdsCallback<CreativeNotificationAdList> callback,
    const BrowsingHistoryList& browsing_history) {
  database::table::CreativeNotificationAds database_table;
  database_table.GetAll([=](const bool success, const SegmentList& /*segments*/,
                            const CreativeNotificationAdList& creative_ads) {
    if (!success) {
      BLOG(1, "Failed to get ads");
      callback(/*had_opportunity*/ false, {});
      return;
    }

    if (creative_ads.empty()) {
      BLOG(1, "No eligible ads");
      callback(/*had_opportunity*/ false, {});
      return;
    }

    const CreativeNotificationAdList eligible_creative_ads =
        FilterCreativeAds(creative_ads, ad_events, browsing_history);
    if (eligible_creative_ads.empty()) {
      BLOG(1, "No eligible ads out of " << creative_ads.size() << " ads");
      callback(/*had_opportunity*/ false, {});
      return;
    }

    PredictAdEmbeddings<CreativeNotificationAdInfo>(user_model, ad_events, eligible_creative_ads,
        [=](const absl::optional<CreativeNotificationAdInfo> creative_ad) {
          if (!creative_ad) {
            BLOG(1, "No eligible ads out of " << creative_ads.size() << " ads");
            callback(/*had_opportunity*/ false, {});
            return;
          }

          BLOG(1, eligible_creative_ads.size()
                      << " eligible ads out of " << creative_ads.size() << " ads");

          callback(/*had_opportunity*/ false, {*creative_ad});
      });
  });
}

CreativeNotificationAdList EligibleAdsV3::FilterCreativeAds(
    const CreativeNotificationAdList& creative_ads,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history) {
  if (creative_ads.empty()) {
    return {};
  }

  ExclusionRules exclusion_rules(ad_events, subdivision_targeting_,
                                 anti_targeting_resource_, browsing_history);
  return ApplyExclusionRules(creative_ads, last_served_ad_, &exclusion_rules);
}

}  // namespace ads::notification_ads
