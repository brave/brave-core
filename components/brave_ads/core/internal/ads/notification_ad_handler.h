/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_NOTIFICATION_AD_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_NOTIFICATION_AD_HANDLER_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom-shared.h"
#include "brave/components/brave_ads/core/ads_client_notifier_observer.h"
#include "brave/components/brave_ads/core/internal/account/account_observer.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/notification_ads/notification_ad_event_handler_observer.h"
#include "brave/components/brave_ads/core/internal/ads/serving/notification_ad_serving_observer.h"
#include "brave/components/brave_ads/core/internal/browser/browser_manager_observer.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

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

class NotificationAdHandler final
    : public AccountObserver,
      public BrowserManagerObserver,
      public notification_ads::EventHandlerObserver,
      public notification_ads::ServingObserver,
      public AdsClientNotifierObserver {
 public:
  NotificationAdHandler(
      Account* account,
      Transfer* transfer,
      processor::EpsilonGreedyBandit* epsilon_greedy_bandit_processor,
      geographic::SubdivisionTargeting* subdivision_targeting,
      resource::AntiTargeting* anti_targeting_resource);

  NotificationAdHandler(const NotificationAdHandler& other) = delete;
  NotificationAdHandler& operator=(const NotificationAdHandler& other) = delete;

  NotificationAdHandler(NotificationAdHandler&& other) noexcept = delete;
  NotificationAdHandler& operator=(NotificationAdHandler&& other) noexcept =
      delete;

  ~NotificationAdHandler() override;

  void MaybeServeAtRegularIntervals();

  void TriggerEvent(const std::string& placement_id,
                    mojom::NotificationAdEventType event_type);

 private:
  // AccountObserver:
  void OnWalletDidUpdate(const WalletInfo& wallet) override;

  // BrowserManagerObserver:
  void OnBrowserDidEnterForeground() override;
  void OnBrowserDidEnterBackground() override;

  // AdsClientNotifierObserver:
  void OnNotifyPrefDidChange(const std::string& path) override;
  void OnNotifyUserDidBecomeActive(base::TimeDelta idle_time,
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

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_NOTIFICATION_AD_HANDLER_H_
