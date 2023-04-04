/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PIPELINES_NOTIFICATION_ADS_ELIGIBLE_NOTIFICATION_ADS_V3_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PIPELINES_NOTIFICATION_ADS_ELIGIBLE_NOTIFICATION_ADS_V3_H_

#include "bat/ads/internal/ads/ad_events/ad_event_info.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_alias.h"
#include "bat/ads/internal/ads/serving/eligible_ads/pipelines/notification_ads/eligible_notification_ads_base.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_info.h"

namespace ads {

namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic

namespace resource {
class AntiTargeting;
}  // namespace resource

namespace targeting {
struct UserModelInfo;
}  // namespace targeting

namespace notification_ads {

class EligibleAdsV3 final : public EligibleAdsBase {
 public:
  EligibleAdsV3(geographic::SubdivisionTargeting* subdivision_targeting,
                resource::AntiTargeting* anti_targeting);

  void GetForUserModel(
      const targeting::UserModelInfo& user_model,
      GetEligibleAdsCallback<CreativeNotificationAdList> callback) override;

 private:
  void GetBrowsingHistory(
      const targeting::UserModelInfo& user_model,
      const AdEventList& ad_events,
      GetEligibleAdsCallback<CreativeNotificationAdList> callback);

  void GetEligibleAds(
      const targeting::UserModelInfo& user_model,
      const AdEventList& ad_events,
      GetEligibleAdsCallback<CreativeNotificationAdList> callback,
      const BrowsingHistoryList& browsing_history);

  CreativeNotificationAdList FilterCreativeAds(
      const CreativeNotificationAdList& creative_ads,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history);
};

}  // namespace notification_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PIPELINES_NOTIFICATION_ADS_ELIGIBLE_NOTIFICATION_ADS_V2_H_
