/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_AD_NOTIFICATIONS_AD_NOTIFICATION_SERVING_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_AD_NOTIFICATIONS_AD_NOTIFICATION_SERVING_H_

#include <memory>

#include "base/time/time.h"
#include "bat/ads/internal/ad_serving/ad_notifications/ad_notification_serving_observer.h"
#include "bat/ads/internal/ads/ad_notifications/ad_notification_observer.h"
#include "bat/ads/internal/bundle/creative_ad_notification_info.h"
#include "bat/ads/internal/timer.h"

namespace ads {

class AdTargeting;

namespace ad_targeting {
namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic
}  // namespace ad_targeting

namespace resource {
class AntiTargeting;
}  // namespace resource

namespace ad_notifications {

class EligibleAds;

class AdServing {
 public:
  AdServing(
      AdTargeting* ad_targeting,
      ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
      resource::AntiTargeting* anti_targeting_resource);

  ~AdServing();

  void AddObserver(AdNotificationServingObserver* observer);
  void RemoveObserver(AdNotificationServingObserver* observer);

  void StartServingAdsAtRegularIntervals();
  void StopServingAdsAtRegularIntervals();

  void MaybeServeAd();

 private:
  Timer timer_;

  AdTargeting* ad_targeting_;  // NOT OWNED

  ad_targeting::geographic::SubdivisionTargeting*
      subdivision_targeting_;  // NOT OWNED

  resource::AntiTargeting* anti_targeting_resource_;  // NOT OWNED

  std::unique_ptr<EligibleAds> eligible_ads_;

  bool ShouldServeAd() const;
  base::Time MaybeServeAfter(const base::TimeDelta delay);

  bool ServeAd(
      const CreativeAdNotificationInfo& creative_ad_notification) const;
  void FailedToServeAd();
  void ServedAd();

  base::ObserverList<AdNotificationServingObserver> observers_;

  void NotifyDidServeAdNotification(const AdNotificationInfo& ad) const;
  void NotifyFailedToServeAdNotification() const;
};

}  // namespace ad_notifications
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_AD_NOTIFICATIONS_AD_NOTIFICATION_SERVING_H_
