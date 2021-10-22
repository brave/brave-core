/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_AD_NOTIFICATIONS_ELIGIBLE_AD_NOTIFICATIONS_V1_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_AD_NOTIFICATIONS_ELIGIBLE_AD_NOTIFICATIONS_V1_H_

#include "bat/ads/internal/ad_events/ad_event_info_aliases.h"
#include "bat/ads/internal/bundle/creative_ad_notification_info_aliases.h"
#include "bat/ads/internal/eligible_ads/ad_notifications/eligible_ad_notifications_base.h"
#include "bat/ads/internal/eligible_ads/eligible_ads_aliases.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_aliases.h"

namespace ads {

namespace ad_targeting {
struct UserModelInfo;
namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic
}  // namespace ad_targeting

namespace resource {
class AntiTargeting;
}  // namespace resource

namespace ad_notifications {

class EligibleAdsV1 final : public EligibleAdsBase {
 public:
  EligibleAdsV1(
      ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
      resource::AntiTargeting* anti_targeting);
  ~EligibleAdsV1() override;

  void GetForUserModel(
      const ad_targeting::UserModelInfo& user_model,
      GetEligibleAdsCallback<CreativeAdNotificationList> callback) override;

 private:
  void GetEligibleAds(
      const ad_targeting::UserModelInfo& user_model,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history,
      GetEligibleAdsCallback<CreativeAdNotificationList> callback);

  void GetForParentChildSegments(
      const ad_targeting::UserModelInfo& user_model,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history,
      GetEligibleAdsCallback<CreativeAdNotificationList> callback);

  void GetForParentSegments(
      const ad_targeting::UserModelInfo& user_model,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history,
      GetEligibleAdsCallback<CreativeAdNotificationList> callback);

  void GetForUntargeted(
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history,
      GetEligibleAdsCallback<CreativeAdNotificationList> callback);

  CreativeAdNotificationList FilterCreativeAds(
      const CreativeAdNotificationList& creative_ads,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history);
};

}  // namespace ad_notifications
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_AD_NOTIFICATIONS_ELIGIBLE_AD_NOTIFICATIONS_V1_H_
