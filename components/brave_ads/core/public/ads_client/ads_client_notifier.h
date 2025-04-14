/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_CLIENT_ADS_CLIENT_NOTIFIER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_CLIENT_ADS_CLIENT_NOTIFIER_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier_interface.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier_observer.h"

class GURL;

namespace brave_ads {

class AdsClientNotifierObserver;
class OnceClosureTaskQueue;

class AdsClientNotifier final : public AdsClientNotifierInterface {
 public:
  AdsClientNotifier();

  AdsClientNotifier(const AdsClientNotifier&) = delete;
  AdsClientNotifier& operator=(const AdsClientNotifier&) = delete;

  ~AdsClientNotifier() override;

  // AdsClientNotifierInterface:
  void AddObserver(AdsClientNotifierObserver* observer) override;
  void RemoveObserver(AdsClientNotifierObserver* observer) override;
  void NotifyPendingObservers() override;
  void NotifyDidInitializeAds() override;
  void NotifyLocaleDidChange(const std::string& locale) override;
  void NotifyPrefDidChange(const std::string& path) override;
  void NotifyResourceComponentDidChange(const std::string& manifest_version,
                                        const std::string& id) override;
  void NotifyDidUnregisterResourceComponent(const std::string& id) override;
  void NotifyRewardsWalletDidUpdate(
      const std::string& payment_id,
      const std::string& recovery_seed_base64) override;
  void NotifyTabTextContentDidChange(int32_t tab_id,
                                     const std::vector<GURL>& redirect_chain,
                                     const std::string& text) override;
  void NotifyTabHtmlContentDidChange(int32_t tab_id,
                                     const std::vector<GURL>& redirect_chain,
                                     const std::string& html) override;
  void NotifyTabDidStartPlayingMedia(int32_t tab_id) override;
  void NotifyTabDidStopPlayingMedia(int32_t tab_id) override;
  void NotifyTabDidChange(int32_t tab_id,
                          const std::vector<GURL>& redirect_chain,
                          bool is_new_navigation,
                          bool is_restoring,
                          bool is_visible) override;
  void NotifyTabDidLoad(int32_t tab_id, int http_status_code) override;
  void NotifyDidCloseTab(int32_t tab_id) override;
  void NotifyUserGestureEventTriggered(int32_t page_transition_type) override;
  void NotifyUserDidBecomeIdle() override;
  void NotifyUserDidBecomeActive(base::TimeDelta idle_time,
                                 bool screen_was_locked) override;
  void NotifyBrowserDidEnterForeground() override;
  void NotifyBrowserDidEnterBackground() override;
  void NotifyBrowserDidBecomeActive() override;
  void NotifyBrowserDidResignActive() override;
  void NotifyDidSolveAdaptiveCaptcha() override;

 private:
  base::ObserverList<AdsClientNotifierObserver> observers_;

  std::unique_ptr<OnceClosureTaskQueue> task_queue_;

  base::WeakPtrFactory<AdsClientNotifier> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_CLIENT_ADS_CLIENT_NOTIFIER_H_
