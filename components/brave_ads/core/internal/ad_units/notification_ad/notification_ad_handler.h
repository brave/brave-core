/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_NOTIFICATION_AD_NOTIFICATION_AD_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_NOTIFICATION_AD_NOTIFICATION_AD_HANDLER_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/application_state/browser_manager_observer.h"
#include "brave/components/brave_ads/core/internal/serving/notification_ad_serving.h"
#include "brave/components/brave_ads/core/internal/serving/notification_ad_serving_delegate.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/notification_ads/notification_ad_event_handler.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/notification_ads/notification_ad_event_handler_delegate.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier_observer.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

class AntiTargetingResource;
class SiteVisit;
class SubdivisionTargeting;
struct NotificationAdInfo;

class NotificationAdHandler final : public AdsClientNotifierObserver,
                                    public BrowserManagerObserver,
                                    public NotificationAdEventHandlerDelegate,
                                    public NotificationAdServingDelegate {
 public:
  NotificationAdHandler(SiteVisit& site_visit,
                        const SubdivisionTargeting& subdivision_targeting,
                        const AntiTargetingResource& anti_targeting_resource);

  NotificationAdHandler(const NotificationAdHandler&) = delete;
  NotificationAdHandler& operator=(const NotificationAdHandler&) = delete;

  NotificationAdHandler(NotificationAdHandler&&) noexcept = delete;
  NotificationAdHandler& operator=(NotificationAdHandler&&) noexcept = delete;

  ~NotificationAdHandler() override;

  void TriggerEvent(const std::string& placement_id,
                    mojom::NotificationAdEventType event_type,
                    TriggerAdEventCallback callback);

 private:
  void MaybeServeAtRegularIntervals();

  void FireServedEventCallback(TriggerAdEventCallback callback,
                               bool success,
                               const std::string& placement_id,
                               mojom::NotificationAdEventType event_type);

  // AdsClientNotifierObserver:
  void OnNotifyDidInitializeAds() override;
  void OnNotifyPrefDidChange(const std::string& path) override;
  void OnNotifyUserDidBecomeActive(base::TimeDelta idle_time,
                                   bool screen_was_locked) override;

  // BrowserManagerObserver:
  void OnBrowserDidEnterForeground() override;
  void OnBrowserDidEnterBackground() override;

  // NotificationAdServingDelegate:
  void OnOpportunityAroseToServeNotificationAd(
      const SegmentList& segments) override;
  void OnDidServeNotificationAd(const NotificationAdInfo& ad) override;

  // NotificationAdEventHandlerDelegate:
  void OnDidFireNotificationAdServedEvent(
      const NotificationAdInfo& ad) override;
  void OnDidFireNotificationAdViewedEvent(
      const NotificationAdInfo& ad) override;
  void OnDidFireNotificationAdClickedEvent(
      const NotificationAdInfo& ad) override;
  void OnDidFireNotificationAdDismissedEvent(
      const NotificationAdInfo& ad) override;
  void OnDidFireNotificationAdTimedOutEvent(
      const NotificationAdInfo& ad) override;

  const raw_ref<SiteVisit> site_visit_;

  NotificationAdEventHandler event_handler_;

  NotificationAdServing serving_;

  base::WeakPtrFactory<NotificationAdHandler> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_NOTIFICATION_AD_NOTIFICATION_AD_HANDLER_H_
