/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_CLIENT_NOTIFIER_IMPL_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_CLIENT_NOTIFIER_IMPL_H_

#include <cstdint>
#include <string>
#include <vector>

#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier_observer.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace bat_ads {

// Receives `BatAdsClientNotifier` Mojo calls from the browser process and
// forwards them to `AdsClientNotifier`.
class BatAdsClientNotifierImpl final
    : public bat_ads::mojom::BatAdsClientNotifier {
 public:
  explicit BatAdsClientNotifierImpl(
      mojo::PendingReceiver<mojom::BatAdsClientNotifier>
          bat_ads_client_notifier_pending_receiver);

  BatAdsClientNotifierImpl(const BatAdsClientNotifierImpl& other) = delete;
  BatAdsClientNotifierImpl& operator=(const BatAdsClientNotifierImpl& other) =
      delete;

  ~BatAdsClientNotifierImpl() override;

  void AddObserver(brave_ads::AdsClientNotifierObserver* observer);
  void RemoveObserver(brave_ads::AdsClientNotifierObserver* observer);

  void NotifyPendingObservers();

  // bat_ads::mojom::BatAdsClientNotifier:
  void NotifyDidInitializeAds() override;
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
  void NotifyTabDidStartPlayingMedia(int32_t tab_id) override;
  void NotifyTabDidStopPlayingMedia(int32_t tab_id) override;
  void NotifyTabDidChange(int32_t tab_id,
                          const std::vector<GURL>& redirect_chain,
                          bool is_new_navigation,
                          bool is_restoring,
                          bool is_visible) override;
  void NotifyTabDidLoad(int32_t tab_id, int http_status_code) override;
  void NotifyDidCloseTab(int32_t tab_id) override;
  void NotifyUserGestureEventTriggered(int32_t page_transition) override;
  void NotifyUserDidBecomeIdle() override;
  void NotifyUserDidBecomeActive(base::TimeDelta idle_time,
                                 bool screen_was_locked) override;
  void NotifyBrowserDidEnterForeground() override;
  void NotifyBrowserDidEnterBackground() override;
  void NotifyBrowserDidBecomeActive() override;
  void NotifyBrowserDidResignActive() override;
  void NotifyDidSolveAdaptiveCaptcha() override;

 private:
  brave_ads::AdsClientNotifier ads_client_notifier_;

  mojo::PendingReceiver<bat_ads::mojom::BatAdsClientNotifier>
      bat_ads_client_notifier_pending_receiver_;
  mojo::Receiver<bat_ads::mojom::BatAdsClientNotifier>
      bat_ads_client_notifier_receiver_{this};
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_CLIENT_NOTIFIER_IMPL_H_
