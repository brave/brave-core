/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_SERVING_AD_NOTIFICATION_SERVING_H_
#define BAT_ADS_INTERNAL_AD_SERVING_AD_NOTIFICATION_SERVING_H_

#include "base/gtest_prod_util.h"
#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/ad_targeting/ad_targeting.h"
#include "bat/ads/internal/bundle/creative_ad_notification_info.h"
#include "bat/ads/internal/timer.h"
#include "bat/ads/result.h"

namespace ads {

class AdsImpl;
struct AdNotificationInfo;

namespace ad_notifications {

using MaybeServeAdForCategoriesCallback =
    std::function<void(const Result, const AdNotificationInfo&)>;

class AdServing {
 public:
  AdServing(
      AdsImpl* ads);

  ~AdServing();

  void ServeAtRegularIntervals();

  void StopServing();

  void MaybeServe();

 private:
  // TODO(https://github.com/brave/brave-browser/issues/12315): Update
  // BatAdsAdNotificationPacingTest to test the contract, not the implementation
  FRIEND_TEST_ALL_PREFIXES(BatAdsAdNotificationPacingTest,
      PacingDisableDelivery);
  FRIEND_TEST_ALL_PREFIXES(BatAdsAdNotificationPacingTest,
      NoPacing);
  FRIEND_TEST_ALL_PREFIXES(BatAdsAdNotificationPacingTest,
      SimplePacing);
  FRIEND_TEST_ALL_PREFIXES(BatAdsAdNotificationPacingTest,
      NoPacingPrioritized);
  FRIEND_TEST_ALL_PREFIXES(BatAdsAdNotificationPacingTest,
      PacingDisableDeliveryPrioritized);
  FRIEND_TEST_ALL_PREFIXES(BatAdsAdNotificationPacingTest,
      PacingAndPrioritization);

  bool NextIntervalHasElapsed();

  base::Time MaybeServeAfter(
      const base::TimeDelta delay);

  void MaybeServeAdForCategories(
      const CategoryList& categories,
      MaybeServeAdForCategoriesCallback callback);

  void MaybeServeAdForParentChildCategories(
      const CategoryList& categories,
      const AdEventList& ad_events,
      MaybeServeAdForCategoriesCallback callback);

  void MaybeServeAdForParentCategories(
      const CategoryList& categories,
      const AdEventList& ad_events,
      MaybeServeAdForCategoriesCallback callback);

  void MaybeServeAdForUntargeted(
      const AdEventList& ad_events,
      MaybeServeAdForCategoriesCallback callback);

  void MaybeServeAd(
      const CreativeAdNotificationList& ads,
      MaybeServeAdForCategoriesCallback callback);

  CreativeAdNotificationList PaceAds(
      const CreativeAdNotificationList& ads);

  void MaybeDeliverAd(
      const CreativeAdNotificationInfo& ad,
      MaybeServeAdForCategoriesCallback callback);

  void FailedToDeliverAd();

  void DeliveredAd();

  void RecordAdOpportunityForCategories(
      const CategoryList& categories);

  Timer timer_;

  CreativeAdInfo last_delivered_creative_ad_;

  AdsImpl* ads_;  // NOT OWNED
};

}  // namespace ad_notifications
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_SERVING_AD_NOTIFICATION_SERVING_H_
