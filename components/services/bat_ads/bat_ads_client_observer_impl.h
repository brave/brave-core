/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_CLIENT_OBSERVER_IMPL_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_CLIENT_OBSERVER_IMPL_H_

#include <cstdint>
#include <string>
#include <vector>

#include "bat/ads/ads_client_observer.h"
#include "bat/ads/ads_client_observer_manager.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace bat_ads {

class BatAdsClientObserverImpl : public bat_ads::mojom::BatAdsClientObserver {
 public:
  BatAdsClientObserverImpl();

  BatAdsClientObserverImpl(const BatAdsClientObserverImpl& other) = delete;
  BatAdsClientObserverImpl& operator=(const BatAdsClientObserverImpl& other) =
      delete;

  BatAdsClientObserverImpl(BatAdsClientObserverImpl&& other) noexcept = delete;
  BatAdsClientObserverImpl& operator=(
      BatAdsClientObserverImpl&& other) noexcept = delete;

  ~BatAdsClientObserverImpl() override;

  // Creates a pending receiver, connecting it to a new PendingRemote which is
  // returned for transmission elsewhere (typically to a Remote who will consume
  // it to start making calls).
  mojo::PendingRemote<bat_ads::mojom::BatAdsClientObserver>
  CreatePendingReceiverAndPassRemote();

  // Binds the receiver by consuming the pending receiver swhich was created.
  void BindReceiver();

  void AddObserver(ads::AdsClientObserver* observer);

  void RemoveObserver(ads::AdsClientObserver* observer);

  // Invoked when the operating system locale changes.
  void NotifyLocaleDidChange(const std::string& locale) override;

  // Invoked when a preference has changed for the specified |path|.
  void NotifyPrefDidChange(const std::string& path) override;

  // Invoked when a resource component has been updated.
  void NotifyDidUpdateResourceComponent(const std::string& id) override;

  // Invoked when the page for |tab_id| has loaded and the content is available
  // for analysis. |redirect_chain| containing a list of redirect URLs that
  // occurred on the way to the current page. The current page is the last one
  // in the list (so even when there's no redirect, there should be one entry in
  // the list). |text| containing the page content as text.
  void NotifyTabTextContentDidChange(int32_t tab_id,
                                     const std::vector<GURL>& redirect_chain,
                                     const std::string& text) override;

  // Invoked when the page for |tab_id| has loaded and the content is available
  // for analysis. |redirect_chain| containing a list of redirect URLs that
  // occurred on the way to the current page. The current page is the last one
  // in the list (so even when there's no redirect, there should be one entry in
  // the list). |html| containing the page content as HTML.
  void NotifyTabHtmlContentDidChange(int32_t tab_id,
                                     const std::vector<GURL>& redirect_chain,
                                     const std::string& html) override;

  // Invoked when media starts playing on a browser tab for the specified
  // |tab_id|.
  void NotifyTabDidStartPlayingMedia(int32_t tab_id) override;

  // Called when media stops playing on a browser tab for the specified
  // |tab_id|.
  void NotifyTabDidStopPlayingMedia(int32_t tab_id) override;

  // Invoked when a browser tab is updated with the specified |redirect_chain|
  // containing a list of redirect URLs that occurred on the way to the current
  // page. The current page is the last one in the list (so even when there's no
  // redirect, there should be one entry in the list). |is_active| is set to
  // |true| if |tab_id| refers to the currently active tab otherwise is set to
  // |false|. |is_browser_active| is set to |true| if the browser window is
  // active otherwise |false|. |is_incognito| is set to |true| if the tab is
  // incognito otherwise |false|.
  void NotifyTabDidChange(int32_t tab_id,
                          const std::vector<GURL>& redirect_chain,
                          bool is_visible,
                          bool is_incognito) override;

  // Invoked when a browser tab with the specified |tab_id| is closed.
  void NotifyDidCloseTab(int32_t tab_id) override;

  // Invoked when a user has been idle for the threshold set in
  // |prefs::kIdleTimeThreshold|. NOTE: This should not be called on mobile
  // devices.
  void NotifyUserDidBecomeIdle() override;

  // Called when a user is no longer idle. |idle_time| is the amount of time in
  // seconds that the user was idle. |screen_was_locked| should be |true| if the
  // screen was locked, otherwise |false|. NOTE: This should not be called on
  // mobile devices.
  void NotifyUserDidBecomeActive(base::TimeDelta idle_time,
                                 bool screen_was_locked) override;

  // Invoked when the browser did enter the foreground.
  void NotifyBrowserDidEnterForeground() override;

  // Invoked when the browser did enter the background.
  void NotifyBrowserDidEnterBackground() override;

  // Invoked when the browser did become active.
  void NotifyBrowserDidBecomeActive() override;

  // Invoked when the browser did resign active.
  void NotifyBrowserDidResignActive() override;

  // Invoked when the user's Brave Rewards wallet is ready.
  void NotifyRewardsWalletIsReady(const std::string& payment_id,
                                  const std::string& recovery_seed) override;

  // Invoked when the user's Brave Rewards wallet has changed.
  void NotifyRewardsWalletDidChange(const std::string& payment_id,
                                    const std::string& recovery_seed) override;

 private:
  ads::AdsClientObserverManager observer_manager_;

  mojo::PendingReceiver<bat_ads::mojom::BatAdsClientObserver> pending_receiver_;
  mojo::Receiver<bat_ads::mojom::BatAdsClientObserver> receiver_{this};
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_CLIENT_OBSERVER_IMPL_H_
