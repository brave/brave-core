/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_NOTIFICATION_AD_SERVING_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_NOTIFICATION_AD_SERVING_H_

#include <memory>
#include <string>

#include "base/check_op.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/ads_client_notifier_observer.h"
#include "brave/components/brave_ads/core/internal/ads/serving/notification_ad_serving_delegate.h"
#include "brave/components/brave_ads/core/internal/common/timer/timer.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"

namespace base {
class Time;
class TimeDelta;
}  // namespace base

namespace brave_ads {

namespace resource {
class AntiTargeting;
}  // namespace resource

namespace targeting {
struct UserModelInfo;
}  // namespace targeting

class SubdivisionTargeting;
struct NotificationAdInfo;

namespace notification_ads {

class EligibleAdsBase;

class Serving final : public AdsClientNotifierObserver {
 public:
  Serving(const SubdivisionTargeting& subdivision_targeting,
          const resource::AntiTargeting& anti_targeting_resource);

  Serving(const Serving&) = delete;
  Serving& operator=(const Serving&) = delete;

  Serving(Serving&&) noexcept = delete;
  Serving& operator=(Serving&&) noexcept = delete;

  ~Serving() override;

  void SetDelegate(ServingDelegate* delegate) {
    DCHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void StartServingAdsAtRegularIntervals();
  void StopServingAdsAtRegularIntervals();

  void MaybeServeAd();

 private:
  bool IsSupported() const { return static_cast<bool>(eligible_ads_); }

  void OnBuildUserModel(const targeting::UserModelInfo& user_model);
  void OnGetForUserModel(const targeting::UserModelInfo& user_model,
                         bool had_opportunity,
                         const CreativeNotificationAdList& creative_ads);

  void OnAdsPerHourPrefChanged();

  void MaybeServeAdAtNextRegularInterval();
  void RetryServingAdAtNextInterval();
  base::Time MaybeServeAdAfter(base::TimeDelta delay);

  void ServeAd(const NotificationAdInfo& ad);
  void FailedToServeAd();

  // AdsClientNotifierObserver:
  void OnNotifyPrefDidChange(const std::string& path) override;

  raw_ptr<ServingDelegate> delegate_ = nullptr;

  bool is_serving_ = false;

  Timer timer_;

  std::unique_ptr<EligibleAdsBase> eligible_ads_;

  base::WeakPtrFactory<Serving> weak_factory_{this};
};

}  // namespace notification_ads
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_NOTIFICATION_AD_SERVING_H_
