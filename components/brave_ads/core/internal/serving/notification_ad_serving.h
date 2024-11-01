/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_NOTIFICATION_AD_SERVING_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_NOTIFICATION_AD_SERVING_H_

#include <memory>
#include <string>

#include "base/check_op.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/brave_ads/core/internal/common/timer/timer.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/serving/notification_ad_serving_delegate.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier_observer.h"

namespace base {
class Time;
class TimeDelta;
}  // namespace base

namespace brave_ads {

class AntiTargetingResource;
class EligibleNotificationAdsBase;
class SubdivisionTargeting;
struct NotificationAdInfo;

class NotificationAdServing final : public AdsClientNotifierObserver {
 public:
  NotificationAdServing(const SubdivisionTargeting& subdivision_targeting,
                        const AntiTargetingResource& anti_targeting_resource);

  NotificationAdServing(const NotificationAdServing&) = delete;
  NotificationAdServing& operator=(const NotificationAdServing&) = delete;

  NotificationAdServing(NotificationAdServing&&) noexcept = delete;
  NotificationAdServing& operator=(NotificationAdServing&&) noexcept = delete;

  ~NotificationAdServing() override;

  void SetDelegate(NotificationAdServingDelegate* delegate) {
    CHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void StartServingAdsAtRegularIntervals();
  void StopServingAdsAtRegularIntervals();

  void MaybeServeAd();
  void MaybeServeAdAtNextRegularInterval();

 private:
  base::expected<void, std::string> CanServeAd() const;

  bool IsSupported() const { return !!eligible_ads_; }

  void GetUserModel();

  void GetEligibleAds(UserModelInfo user_model);
  void GetEligibleAdsCallback(const CreativeNotificationAdList& creative_ads);

  void UpdateMaximumAdsPerHour();

  void RetryServingAdAtNextInterval();
  base::Time MaybeServeAdAfter(base::TimeDelta delay);

  void ServeAd(const NotificationAdInfo& ad);
  void SuccessfullyServedAd(const NotificationAdInfo& ad);
  void FailedToServeAd();

  void NotifyOpportunityAroseToServeNotificationAd(
      const SegmentList& segments) const;
  void NotifyDidServeNotificationAd(const NotificationAdInfo& ad) const;
  void NotifyFailedToServeNotificationAd() const;

  // AdsClientNotifierObserver:
  void OnNotifyPrefDidChange(const std::string& path) override;

  raw_ptr<NotificationAdServingDelegate> delegate_ = nullptr;  // Not owned.

  bool is_serving_ = false;

  Timer timer_;

  std::unique_ptr<EligibleNotificationAdsBase> eligible_ads_;

  base::WeakPtrFactory<NotificationAdServing> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_NOTIFICATION_AD_SERVING_H_
