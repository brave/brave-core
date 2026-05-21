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
#include "brave/components/brave_ads/buildflags/buildflags.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier_observer.h"
#include "ui/base/page_transition_types.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_ADS));

class GURL;

namespace brave_ads {

class OnceClosureTaskQueue;

class AdsClientNotifier final {
 public:
  AdsClientNotifier();

  AdsClientNotifier(const AdsClientNotifier&) = delete;
  AdsClientNotifier& operator=(const AdsClientNotifier&) = delete;

  ~AdsClientNotifier();

  void AddObserver(AdsClientNotifierObserver* observer);
  void RemoveObserver(AdsClientNotifierObserver* observer);
  void NotifyPendingObservers();
  void NotifyDidInitializeAds();
  void NotifyPrefDidChange(const std::string& path);
  void NotifyResourceComponentDidChange(const std::string& manifest_version,
                                        const std::string& id);
  void NotifyDidUnregisterResourceComponent(const std::string& id);
  void NotifyRewardsWalletDidUpdate(const std::string& payment_id,
                                    const std::string& recovery_seed_base64);
  void NotifyTabTextContentDidChange(int32_t tab_id,
                                     const std::vector<GURL>& redirect_chain,
                                     const std::string& text);
  void NotifyTabDidStartPlayingMedia(int32_t tab_id);
  void NotifyTabDidStopPlayingMedia(int32_t tab_id);
  void NotifyTabDidChange(int32_t tab_id,
                          const std::vector<GURL>& redirect_chain,
                          bool is_new_navigation,
                          bool is_restoring,
                          bool is_visible);
  void NotifyTabDidLoad(int32_t tab_id, int http_status_code);
  void NotifyDidCloseTab(int32_t tab_id);
  void NotifyUserGestureEventTriggered(ui::PageTransition page_transition);
  void NotifyUserDidBecomeIdle();
  void NotifyUserDidBecomeActive(base::TimeDelta idle_time,
                                 bool screen_was_locked);
  void NotifyBrowserDidEnterForeground();
  void NotifyBrowserDidEnterBackground();
  void NotifyBrowserDidBecomeActive();
  void NotifyBrowserDidResignActive();
  void NotifyDidSolveAdaptiveCaptcha();

 private:
  base::ObserverList<AdsClientNotifierObserver> observers_;

  std::unique_ptr<OnceClosureTaskQueue> task_queue_;

  base::WeakPtrFactory<AdsClientNotifier> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_CLIENT_ADS_CLIENT_NOTIFIER_H_
