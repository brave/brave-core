/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_AD_NOTIFICATIONS_AD_NOTIFICATION_SERVING_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_AD_NOTIFICATIONS_AD_NOTIFICATION_SERVING_H_

#include <memory>
#include <string>

#include "base/observer_list.h"
#include "bat/ads/internal/ad_serving/ad_notifications/ad_notification_serving_observer.h"
#include "bat/ads/internal/timer.h"

namespace base {
class Time;
class TimeDelta;
}  // namespace base

namespace ads {

namespace ad_targeting {
namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic
}  // namespace ad_targeting

namespace resource {
class AntiTargeting;
}  // namespace resource

struct AdNotificationInfo;

namespace ad_notifications {

class EligibleAdsBase;

class AdServing final {
 public:
  AdServing(
      ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
      resource::AntiTargeting* anti_targeting_resource);
  ~AdServing();

  void AddObserver(AdNotificationServingObserver* observer);
  void RemoveObserver(AdNotificationServingObserver* observer);

  void StartServingAdsAtRegularIntervals();
  void StopServingAdsAtRegularIntervals();

  void MaybeServeAd();

  void OnPrefChanged(const std::string& path);

 private:
  bool IsSupported() const;

  bool ShouldServeAdsAtRegularIntervals() const;
  bool HasPreviouslyServedAnAd() const;
  bool ShouldServeAd() const;
  base::TimeDelta CalculateDelayBeforeServingAnAd() const;
  void MaybeServeAdAtNextRegularInterval();
  void RetryServingAdAtNextInterval();
  base::Time MaybeServeAdAfter(const base::TimeDelta delay);

  bool ServeAd(const AdNotificationInfo& ad) const;
  void FailedToServeAd();
  void ServedAd(const AdNotificationInfo& ad);

  void NotifyDidServeAdNotification(const AdNotificationInfo& ad) const;
  void NotifyFailedToServeAdNotification() const;

  base::ObserverList<AdNotificationServingObserver> observers_;

  bool is_serving_ = false;

  Timer timer_;

  std::unique_ptr<EligibleAdsBase> eligible_ads_;
};

}  // namespace ad_notifications
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_AD_NOTIFICATIONS_AD_NOTIFICATION_SERVING_H_
