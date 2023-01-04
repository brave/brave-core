/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_NOTIFICATION_AD_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_NOTIFICATION_AD_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "bat/ads/internal/account/account_observer.h"
#include "bat/ads/internal/ads/ad_events/notification_ads/notification_ad_event_handler_observer.h"
#include "bat/ads/internal/ads/serving/notification_ad_serving_observer.h"
#include "bat/ads/internal/browser/browser_manager_observer.h"
#include "bat/ads/internal/prefs/pref_manager_observer.h"
#include "bat/ads/internal/segments/segment_alias.h"
#include "bat/ads/internal/user_interaction/idle_detection/idle_detection_manager_observer.h"
#include "bat/ads/public/interfaces/ads.mojom-shared.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace ads {

namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic

namespace notification_ads {
class EventHandler;
class Serving;
}  // namespace notification_ads

namespace resource {
class AntiTargeting;
}  // namespace resource

namespace processor {
class EpsilonGreedyBandit;
}  // namespace processor

class Account;
class Transfer;
struct NotificationAdInfo;
struct WalletInfo;

class NotificationAd final : public AccountObserver,
                             public BrowserManagerObserver,
                             public IdleDetectionManagerObserver,
                             public notification_ads::EventHandlerObserver,
                             public notification_ads::ServingObserver,
                             public PrefManagerObserver {
 public:
  NotificationAd(
      Account* account,
      Transfer* transfer,
      processor::EpsilonGreedyBandit* epsilon_greedy_bandit_processor,
      geographic::SubdivisionTargeting* subdivision_targeting,
      resource::AntiTargeting* anti_targeting_resource);

  NotificationAd(const NotificationAd& other) = delete;
  NotificationAd& operator=(const NotificationAd& other) = delete;

  NotificationAd(NotificationAd&& other) noexcept = delete;
  NotificationAd& operator=(NotificationAd&& other) noexcept = delete;

  ~NotificationAd() override;

  void MaybeServeAtRegularIntervals();

  void TriggerEvent(const std::string& placement_id,
                    mojom::NotificationAdEventType event_type);

 private:
  // AccountObserver:
  void OnWalletDidUpdate(const WalletInfo& wallet) override;

  // BrowserManagerObserver:
  void OnBrowserDidEnterForeground() override;
  void OnBrowserDidEnterBackground() override;

  // PrefManagerObserver:
  void OnPrefDidChange(const std::string& path) override;

  // IdleDetectionManagerObserver:
  void OnUserDidBecomeActive(base::TimeDelta idle_time,
                             bool screen_was_locked) override;

  // notification_ads::ServingObserver:
  void OnOpportunityAroseToServeNotificationAd(
      const SegmentList& segments) override;
  void OnDidServeNotificationAd(const NotificationAdInfo& ad) override;

  // notification_ads::EventHandlerObserver:
  void OnNotificationAdServed(const NotificationAdInfo& ad) override;
  void OnNotificationAdViewed(const NotificationAdInfo& ad) override;
  void OnNotificationAdClicked(const NotificationAdInfo& ad) override;
  void OnNotificationAdDismissed(const NotificationAdInfo& ad) override;
  void OnNotificationAdTimedOut(const NotificationAdInfo& ad) override;

  std::unique_ptr<notification_ads::EventHandler> event_handler_;

  std::unique_ptr<notification_ads::Serving> serving_;

  const raw_ptr<Account> account_ = nullptr;    // NOT OWNED
  const raw_ptr<Transfer> transfer_ = nullptr;  // NOT OWNED
  const raw_ptr<processor::EpsilonGreedyBandit>
      epsilon_greedy_bandit_processor_ = nullptr;  // NOT OWNED
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_NOTIFICATION_AD_H_
