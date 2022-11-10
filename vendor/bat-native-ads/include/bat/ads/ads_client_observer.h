/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_CLIENT_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_CLIENT_OBSERVER_H_

#include <cstdint>
#include <string>
#include <vector>

#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace ads {

class AdsClientObserver : public bat_ads::mojom::BatAdsClientObserver {
 public:
  AdsClientObserver();

  AdsClientObserver(const AdsClientObserver& other) = delete;
  AdsClientObserver& operator=(const AdsClientObserver& other) = delete;

  AdsClientObserver(AdsClientObserver&& other) noexcept = delete;
  AdsClientObserver& operator=(AdsClientObserver&& other) noexcept = delete;

  ~AdsClientObserver() override;

  // Binds the receiver, connecting it to a new PendingRemote which is returned
  // for transmission elsewhere (typically to a Remote who will consume it to
  // start making calls).
  mojo::PendingRemote<bat_ads::mojom::BatAdsClientObserver> Bind();

  // Indicates whether the receiver is bound, meaning it may continue to receive
  // Interface method calls from a remote caller.
  bool IsBound() const { return receiver_.is_bound(); }

  // Resets the receiver to an unbound state. An unbound Receiver will NEVER
  // schedule method calls or disconnection notifications, and any pending tasks
  // which were scheduled prior to unbinding are effectively cancelled.
  void Reset() { receiver_.reset(); }

  // Invoked when the operating system locale changes.
  void OnLocaleDidChange(const std::string& locale) override {}

  // Invoked when a preference has changed for the specified |path|.
  void OnPrefDidChange(const std::string& path) override {}

  // Invoked when a resource component has been updated.
  void OnDidUpdateResourceComponent(const std::string& id) override {}

  // Invoked when the page for |tab_id| has loaded and the content is available
  // for analysis. |redirect_chain| containing a list of redirect URLs that
  // occurred on the way to the current page. The current page is the last one
  // in the list (so even when there's no redirect, there should be one entry in
  // the list). |text| containing the page content as text.
  void OnTabTextContentDidChange(int32_t tab_id,
                                 const std::vector<GURL>& redirect_chain,
                                 const std::string& text) override {}

  // Invoked when the page for |tab_id| has loaded and the content is available
  // for analysis. |redirect_chain| containing a list of redirect URLs that
  // occurred on the way to the current page. The current page is the last one
  // in the list (so even when there's no redirect, there should be one entry in
  // the list). |html| containing the page content as HTML.
  void OnTabHtmlContentDidChange(int32_t tab_id,
                                 const std::vector<GURL>& redirect_chain,
                                 const std::string& html) override {}

  // Invoked when media starts playing on a browser tab for the specified
  // |tab_id|.
  void OnTabDidStartPlayingMedia(int32_t tab_id) override {}

  // Called when media stops playing on a browser tab for the specified
  // |tab_id|.
  void OnTabDidStopPlayingMedia(int32_t tab_id) override {}

  // Invoked when a browser tab is updated with the specified |redirect_chain|
  // containing a list of redirect URLs that occurred on the way to the current
  // page. The current page is the last one in the list (so even when there's no
  // redirect, there should be one entry in the list). |is_active| is set to
  // |true| if |tab_id| refers to the currently active tab otherwise is set to
  // |false|. |is_browser_active| is set to |true| if the browser window is
  // active otherwise |false|. |is_incognito| is set to |true| if the tab is
  // incognito otherwise |false|.
  void OnTabDidChange(int32_t tab_id,
                      const std::vector<GURL>& redirect_chain,
                      bool is_visible,
                      bool is_incognito) override {}

  // Invoked when a browser tab with the specified |tab_id| is closed.
  void OnDidCloseTab(int32_t tab_id) override {}

  // Invoked when a user has been idle for the threshold set in
  // |prefs::kIdleTimeThreshold|. NOTE: This should not be called on mobile
  // devices.
  void OnUserDidBecomeIdle() override {}

  // Called when a user is no longer idle. |idle_time| is the amount of time in
  // seconds that the user was idle. |screen_was_locked| should be |true| if the
  // screen was locked, otherwise |false|. NOTE: This should not be called on
  // mobile devices.
  void OnUserDidBecomeActive(base::TimeDelta idle_time,
                             bool screen_was_locked) override {}

  // Invoked when the browser did enter the foreground.
  void OnBrowserDidEnterForeground() override {}

  // Invoked when the browser did enter the background.
  void OnBrowserDidEnterBackground() override {}

  // Invoked when the browser did become active.
  void OnBrowserDidBecomeActive() override {}

  // Invoked when the browser did resign active.
  void OnBrowserDidResignActive() override {}

  // Invoked when the user's Brave Rewards wallet is ready.
  void OnRewardsWalletIsReady(const std::string& payment_id,
                              const std::string& recovery_seed) override {}

  // Invoked when the user's Brave Rewards wallet has changed.
  void OnRewardsWalletDidChange(const std::string& payment_id,
                                const std::string& recovery_seed) override {}

 private:
  mojo::Receiver<bat_ads::mojom::BatAdsClientObserver> receiver_{this};
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_CLIENT_OBSERVER_H_
