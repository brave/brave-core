/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/inline_content_ads/eligible_inline_content_ads_v2.h"

#include "base/check.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/ad_serving/ad_serving_features.h"
#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_user_model_info.h"
#include "bat/ads/internal/ads/inline_content_ads/inline_content_ad_exclusion_rules.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/bundle/creative_ad_notification_info.h"
#include "bat/ads/internal/database/tables/ad_events_database_table.h"
#include "bat/ads/internal/database/tables/creative_inline_content_ads_database_table.h"
#include "bat/ads/internal/eligible_ads/choose_ad.h"
#include "bat/ads/internal/eligible_ads/frequency_capping.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting/anti_targeting_resource.h"
#include "bat/ads/internal/segments/segments_aliases.h"

namespace ads {
namespace inline_content_ads {

EligibleAdsV2::EligibleAdsV2(
    ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
    resource::AntiTargeting* anti_targeting_resource)
    : EligibleAdsBase(subdivision_targeting, anti_targeting_resource) {}

EligibleAdsV2::~EligibleAdsV2() = default;

void EligibleAdsV2::GetForUserModel(
    const ad_targeting::UserModelInfo& user_model,
    const std::string& dimensions,
    GetEligibleAdsCallback<CreativeInlineContentAdList> callback) {
  BLOG(1, "Get eligible inline content ads:");

  database::table::AdEvents database_table;
  database_table.GetForType(
      mojom::AdType::kInlineContentAd,
      [=](const bool success, const AdEventList& ad_events) {
        if (!success) {
          BLOG(1, "Failed to get ad events");
          callback(/* had_opportunity */ false, {});
          return;
        }

        const int max_count = features::GetBrowsingHistoryMaxCount();
        const int days_ago = features::GetBrowsingHistoryDaysAgo();
        AdsClientHelper::Get()->GetBrowsingHistory(
            max_count, days_ago,
            [=](const BrowsingHistoryList& browsing_history) {
              GetEligibleAds(user_model, ad_events, browsing_history,
                             dimensions, callback);
            });
      });
}

///////////////////////////////////////////////////////////////////////////////

void EligibleAdsV2::GetEligibleAds(
    const ad_targeting::UserModelInfo& user_model,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history,
    const std::string& dimensions,
    GetEligibleAdsCallback<CreativeInlineContentAdList> callback) {
  database::table::CreativeInlineContentAds database_table;
  database_table.GetForDimensions(
      dimensions,
      [=](const bool success, const CreativeInlineContentAdList& creative_ads) {
        if (!success) {
          BLOG(1, "Failed to get ads");
          callback(/* had_opportunity */ false, {});
          return;
        }

        const CreativeInlineContentAdList& eligible_creative_ads =
            FilterCreativeAds(creative_ads, ad_events, browsing_history);
        if (eligible_creative_ads.empty()) {
          BLOG(1, "No eligible ads");
          callback(/* had_opportunity */ true, {});
          return;
        }

        const absl::optional<CreativeInlineContentAdInfo>&
            creative_ad_optional =
                ChooseAd(user_model, ad_events, eligible_creative_ads);
        if (!creative_ad_optional) {
          BLOG(1, "No eligible ads");
          callback(/* had_opportunity */ true, {});
          return;
        }

        const CreativeInlineContentAdInfo& creative_ad =
            creative_ad_optional.value();

        callback(/* had_opportunity */ true, {creative_ad});
      });
}

CreativeInlineContentAdList EligibleAdsV2::FilterCreativeAds(
    const CreativeInlineContentAdList& creative_ads,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history) {
  if (creative_ads.empty()) {
    return {};
  }

  frequency_capping::ExclusionRules exclusion_rules(
      ad_events, subdivision_targeting_, anti_targeting_resource_,
      browsing_history);
  const CreativeInlineContentAdList& eligible_creative_ads =
      ApplyFrequencyCapping(creative_ads, last_served_ad_, &exclusion_rules);

  return eligible_creative_ads;
}

}  // namespace inline_content_ads
}  // namespace ads
