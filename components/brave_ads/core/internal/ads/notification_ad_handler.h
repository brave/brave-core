/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_NOTIFICATION_AD_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_NOTIFICATION_AD_HANDLER_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom-shared.h"
#include "brave/components/brave_ads/core/ads_client_notifier_observer.h"
#include "brave/components/brave_ads/core/internal/account/account_observer.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/notification_ads/notification_ad_event_handler.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/notification_ads/notification_ad_event_handler_delegate.h"
#include "brave/components/brave_ads/core/internal/ads/serving/notification_ad_serving.h"
#include "brave/components/brave_ads/core/internal/ads/serving/notification_ad_serving_delegate.h"
#include "brave/components/brave_ads/core/internal/browser/browser_manager_observer.h"
#include "brave/components/brave_ads/core/internal/processors/behavioral/multi_armed_bandits/epsilon_greedy_bandit_processor.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

class Account;
class AntiTargetingResource;
class SubdivisionTargeting;
class Transfer;
struct NotificationAdInfo;
struct WalletInfo;

class NotificationAdHandler final : public AccountObserver,
                                    public AdsClientNotifierObserver,
                                    public BrowserManagerObserver,
                                    public NotificationAdEventHandlerDelegate,
                                    public NotificationAdServingDelegate {
 public:
  NotificationAdHandler(Account& account,
                        Transfer& transfer,
                        const SubdivisionTargeting& subdivision_targeting,
                        const AntiTargetingResource& anti_targeting_resource);

  NotificationAdHandler(const NotificationAdHandler&) = delete;
  NotificationAdHandler& operator=(const NotificationAdHandler&) = delete;

  NotificationAdHandler(NotificationAdHandler&&) noexcept = delete;
  NotificationAdHandler& operator=(NotificationAdHandler&&) noexcept = delete;

  ~NotificationAdHandler() override;

  void MaybeServeAtRegularIntervals();

  void TriggerEvent(const std::string& placement_id,
                    mojom::NotificationAdEventType event_type);

 private:
  // AccountObserver:
  void OnWalletDidUpdate(const WalletInfo& wallet) override;

  // AdsClientNotifierObserver:
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
  void OnNotificationAdServed(const NotificationAdInfo& ad) override;
  void OnNotificationAdViewed(const NotificationAdInfo& ad) override;
  void OnNotificationAdClicked(const NotificationAdInfo& ad) override;
  void OnNotificationAdDismissed(const NotificationAdInfo& ad) override;
  void OnNotificationAdTimedOut(const NotificationAdInfo& ad) override;

  const raw_ref<Account> account_;
  const raw_ref<Transfer> transfer_;

  NotificationAdEventHandler event_handler_;

  NotificationAdServing serving_;

  const EpsilonGreedyBanditProcessor epsilon_greedy_bandit_processor_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_NOTIFICATION_AD_HANDLER_H_
