/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_CLIENT_OBSERVER_NOTIFIER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_CLIENT_OBSERVER_NOTIFIER_H_

#include <cstdint>
#include <string>
#include <vector>

#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "mojo/public/cpp/bindings/remote_set.h"

namespace ads {

class AdsClientObserverNotifier {
 public:
  AdsClientObserverNotifier();

  AdsClientObserverNotifier(const AdsClientObserverNotifier& other) = delete;
  AdsClientObserverNotifier& operator=(const AdsClientObserverNotifier& other) = delete;

  AdsClientObserverNotifier(AdsClientObserverNotifier&& other) noexcept = delete;
  AdsClientObserverNotifier& operator=(AdsClientObserverNotifier&& other) noexcept = delete;

  ~AdsClientObserverNotifier();

  void AddBatAdsClientObserver(
      mojo::PendingRemote<bat_ads::mojom::BatAdsClientObserver> observer);

  // Invoked when the operating system locale changes.
  void NotifyLocaleDidChange(const std::string& locale) const;

  // Invoked when a preference has changed for the specified |path|.
  void NotifyPrefDidChange(const std::string& path) const;

  // Invoked when a resource component has been updated.
  void NotifyDidUpdateResourceComponent(const std::string& id) const;

  // Invoked when the page for |tab_id| has loaded and the content is available
  // for analysis. |redirect_chain| containing a list of redirect URLs that
  // occurred on the way to the current page. The current page is the last one
  // in the list (so even when there's no redirect, there should be one entry in
  // the list). |text| containing the page content as text.
  void NotifyTabTextContentDidChange(int32_t tab_id,
                                     const std::vector<GURL>& redirect_chain,
                                     const std::string& text) const;

  // Invoked when the page for |tab_id| has loaded and the content is available
  // for analysis. |redirect_chain| containing a list of redirect URLs that
  // occurred on the way to the current page. The current page is the last one
  // in the list (so even when there's no redirect, there should be one entry in
  // the list). |html| containing the page content as HTML.
  void NotifyTabHtmlContentDidChange(int32_t tab_id,
                                     const std::vector<GURL>& redirect_chain,
                                     const std::string& html) const;

  // Invoked when media starts playing on a browser tab for the specified
  // |tab_id|.
  void NotifyTabDidStartPlayingMedia(int32_t tab_id) const;

  // Called when media stops playing on a browser tab for the specified
  // |tab_id|.
  void NotifyTabDidStopPlayingMedia(int32_t tab_id) const;

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
                          bool is_incognito) const;

  // Invoked when a browser tab with the specified |tab_id| is closed.
  void NotifyDidCloseTab(int32_t tab_id) const;

  // Invoked when a user has been idle for the threshold set in
  // |prefs::kIdleTimeThreshold|. NOTE: This should not be called on mobile
  // devices.
  void NotifyUserDidBecomeIdle() const;

  // Called when a user is no longer idle. |idle_time| is the amount of time in
  // seconds that the user was idle. |screen_was_locked| should be |true| if the
  // screen was locked, otherwise |false|. NOTE: This should not be called on
  // mobile devices.
  void NotifyUserDidBecomeActive(base::TimeDelta idle_time,
                                 bool screen_was_locked) const;

  // Invoked when the browser did enter the foreground.
  void NotifyBrowserDidEnterForeground() const;

  // Invoked when the browser did enter the background.
  void NotifyBrowserDidEnterBackground() const;

  // Invoked when the browser did become active.
  void NotifyBrowserDidBecomeActive() const;

  // Invoked when the browser did resign active.
  void NotifyBrowserDidResignActive() const;

  // Invoked when the user's Brave Rewards wallet is ready.
  void NotifyRewardsWalletIsReady(const std::string& payment_id,
                                  const std::string& recovery_seed) const;

  // Invoked when the user's Brave Rewards wallet has changed.
  void NotifyRewardsWalletDidChange(const std::string& payment_id,
                                    const std::string& recovery_seed) const;

 protected:
  mojo::RemoteSet<bat_ads::mojom::BatAdsClientObserver> observers_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_CLIENT_OBSERVER_NOTIFIER_H_
