/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_AD_NOTIFICATIONS_AD_NOTIFICATION_SERVING_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_AD_NOTIFICATIONS_AD_NOTIFICATION_SERVING_H_

#include "base/gtest_prod_util.h"
#include "base/time/time.h"
#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/ad_targeting/ad_targeting.h"
#include "bat/ads/internal/bundle/creative_ad_notification_info.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_aliases.h"
#include "bat/ads/internal/timer.h"
#include "bat/ads/result.h"

namespace ads {

struct AdNotificationInfo;

namespace ad_targeting {
namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic
}  // namespace ad_targeting

namespace resource {
class AntiTargeting;
}  // namespace resource

namespace ad_notifications {

using MaybeServeAdForSegmentsCallback =
    std::function<void(const Result, const AdNotificationInfo&)>;

class AdServing {
 public:
  AdServing(
      AdTargeting* ad_targeting,
      ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
      resource::AntiTargeting* anti_targeting);

  ~AdServing();

  void ServeAtRegularIntervals();
  void StopServing();
  void MaybeServe();

  void OnAdsPerHourChanged();

 private:
  // TODO(https://github.com/brave/brave-browser/issues/12315): Update
  // BatAdsAdNotificationPacingTest to test the contract, not the implementation
  FRIEND_TEST_ALL_PREFIXES(BatAdsAdNotificationPacingTest,
                           PacingDisableDelivery);
  FRIEND_TEST_ALL_PREFIXES(BatAdsAdNotificationPacingTest, NoPacing);
  FRIEND_TEST_ALL_PREFIXES(BatAdsAdNotificationPacingTest, SimplePacing);
  FRIEND_TEST_ALL_PREFIXES(BatAdsAdNotificationPacingTest, NoPacingPrioritized);
  FRIEND_TEST_ALL_PREFIXES(BatAdsAdNotificationPacingTest,
                           PacingDisableDeliveryPrioritized);
  FRIEND_TEST_ALL_PREFIXES(BatAdsAdNotificationPacingTest,
                           PacingAndPrioritization);

  bool NextIntervalHasElapsed();

  void MaybeServeNextAd();

  base::Time MaybeServeAfter(const base::TimeDelta delay);

  void MaybeServeAdForSegments(const SegmentList& segments,
                               MaybeServeAdForSegmentsCallback callback);

  void MaybeServeAdForParentChildSegments(
      const SegmentList& segments,
      const AdEventList& ad_events,
      const BrowsingHistoryList& history,
      MaybeServeAdForSegmentsCallback callback);

  void MaybeServeAdForParentSegments(const SegmentList& segments,
                                     const AdEventList& ad_events,
                                     const BrowsingHistoryList& history,
                                     MaybeServeAdForSegmentsCallback callback);

  void MaybeServeAdForUntargeted(const AdEventList& ad_events,
                                 const BrowsingHistoryList& history,
                                 MaybeServeAdForSegmentsCallback callback);

  void MaybeServeAd(const CreativeAdNotificationList& ads,
                    MaybeServeAdForSegmentsCallback callback);

  CreativeAdNotificationList PaceAds(const CreativeAdNotificationList& ads);

  void MaybeDeliverAd(const CreativeAdNotificationInfo& ad,
                      MaybeServeAdForSegmentsCallback callback);

  void FailedToDeliverAd();

  void DeliveredAd();

  void RecordAdOpportunityForSegments(const SegmentList& segments);

  Timer timer_;

  CreativeAdInfo last_delivered_creative_ad_;

  AdTargeting* ad_targeting_;  // NOT OWNED

  ad_targeting::geographic::SubdivisionTargeting*
      subdivision_targeting_;  // NOT OWNED

  resource::AntiTargeting* anti_targeting_resource_;  // NOT OWNED
};

}  // namespace ad_notifications
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_AD_NOTIFICATIONS_AD_NOTIFICATION_SERVING_H_
