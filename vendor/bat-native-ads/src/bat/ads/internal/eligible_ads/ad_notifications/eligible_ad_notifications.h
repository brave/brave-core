/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_AD_NOTIFICATIONS_ELIGIBLE_AD_NOTIFICATIONS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_AD_NOTIFICATIONS_ELIGIBLE_AD_NOTIFICATIONS_H_

#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/bundle/creative_ad_notification_info.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_aliases.h"

namespace ads {

namespace ad_targeting {
namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic
}  // namespace ad_targeting

namespace resource {
class AntiTargeting;
}  // namespace resource

namespace ad_notifications {

class EligibleAds {
 public:
  EligibleAds(
      ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
      resource::AntiTargeting* anti_targeting);

  ~EligibleAds();

  CreativeAdNotificationList Get(const CreativeAdNotificationList& ads,
                                 const CreativeAdInfo& last_delivered_ad,
                                 const AdEventList& ad_events,
                                 const BrowsingHistoryList& history);

 private:
  ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting_;

  resource::AntiTargeting* anti_targeting_;

  CreativeAdNotificationList RemoveSeenAdvertisersAndRoundRobinIfNeeded(
      const CreativeAdNotificationList& ads) const;

  CreativeAdNotificationList RemoveSeenAdsAndRoundRobinIfNeeded(
      const CreativeAdNotificationList& ads) const;

  CreativeAdNotificationList FrequencyCap(
      const CreativeAdNotificationList& ads,
      const CreativeAdInfo& last_delivered_ad,
      const AdEventList& ad_events,
      const BrowsingHistoryList& history) const;
};

}  // namespace ad_notifications
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_AD_NOTIFICATIONS_ELIGIBLE_AD_NOTIFICATIONS_H_
