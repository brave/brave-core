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

class BatAdsClientNotifierImpl : public bat_ads::mojom::BatAdsClientNotifier {
 public:
  explicit BatAdsClientNotifierImpl(
      mojo::PendingReceiver<mojom::BatAdsClientNotifier> client_notifier);

  BatAdsClientNotifierImpl(const BatAdsClientNotifierImpl& other) = delete;
  BatAdsClientNotifierImpl& operator=(const BatAdsClientNotifierImpl& other) =
      delete;

  BatAdsClientNotifierImpl(BatAdsClientNotifierImpl&& other) noexcept = delete;
  BatAdsClientNotifierImpl& operator=(
      BatAdsClientNotifierImpl&& other) noexcept = delete;

  ~BatAdsClientNotifierImpl() override;

  void AddObserver(brave_ads::AdsClientNotifierObserver* observer);
  void RemoveObserver(brave_ads::AdsClientNotifierObserver* observer);

  // Binds the receiver by consuming the pending receiver swhich was created.
  void BindReceiver();

  // Invoked when ads did initialize.
  void NotifyDidInitializeAds() override;

  // Invoked when the operating system locale changes.
  void NotifyLocaleDidChange(const std::string& locale) override;

  // Invoked when a preference has changed for the specified `path`.
  void NotifyPrefDidChange(const std::string& path) override;

  // Invoked when a resource component with `id` has been updated to
  // `manifest_version`.
  void NotifyResourceComponentDidChange(const std::string& manifest_version,
                                        const std::string& id) override;

  // Invoked when a resource component with `id` has been unregistered.
  void NotifyDidUnregisterResourceComponent(const std::string& id) override;

  // Invoked when the Brave Reward wallet did change.
  void NotifyRewardsWalletDidUpdate(const std::string& payment_id,
                                    const std::string& recovery_seed) override;

  // Invoked when the page for `tab_id` has loaded and the content is available
  // for analysis. `redirect_chain` containing a list of redirect URLs that
  // occurred on the way to the current page. The current page is the last one
  // in the list (so even when there's no redirect, there should be one entry in
  // the list). `text` containing the page content as text.
  void NotifyTabTextContentDidChange(int32_t tab_id,
                                     const std::vector<GURL>& redirect_chain,
                                     const std::string& text) override;

  // Invoked when the page for `tab_id` has loaded and the content is available
  // for analysis. `redirect_chain` containing a list of redirect URLs that
  // occurred on the way to the current page. The current page is the last one
  // in the list (so even when there's no redirect, there should be one entry in
  // the list). `html` containing the page content as HTML.
  void NotifyTabHtmlContentDidChange(int32_t tab_id,
                                     const std::vector<GURL>& redirect_chain,
                                     const std::string& html) override;

  // Invoked when media starts playing on a browser tab for the specified
  // `tab_id`.
  void NotifyTabDidStartPlayingMedia(int32_t tab_id) override;

  // Called when media stops playing on a browser tab for the specified
  // `tab_id`.
  void NotifyTabDidStopPlayingMedia(int32_t tab_id) override;

  // Invoked when a browser tab is updated with the specified `redirect_chain`
  // containing a list of redirect URLs that occurred on the way to the current
  // page. The current page is the last one in the list (so even when there's no
  // redirect, there should be one entry in the list). `is_restoring` should be
  // set to `true` if the page is restoring otherwise should be set to `false`.
  // `is_error_page` should be set to `true` if an error occurred otherwise
  // should be set to `false`. `is_visible` should be set to `true` if `tab_id`
  // refers to the currently visible tab otherwise should be set to `false`.
  void NotifyTabDidChange(int32_t tab_id,
                          const std::vector<GURL>& redirect_chain,
                          bool is_new_navigation,
                          bool is_restoring,
                          bool is_error_page,
                          bool is_visible) override;

  // Invoked when a browser tab with the specified `tab_id` is closed.
  void NotifyDidCloseTab(int32_t tab_id) override;

  // Called when a page navigation was initiated by a user gesture.
  // `page_transition_type` containing the page transition type, see enums for
  // `PageTransitionType`.
  void NotifyUserGestureEventTriggered(int32_t page_transition_type) override;

  // Invoked when a user has been idle for the threshold set in
  // `prefs::kIdleTimeThreshold`. NOTE: This should not be called on mobile
  // devices.
  void NotifyUserDidBecomeIdle() override;

  // Called when a user is no longer idle. `idle_time` is the amount of time in
  // seconds that the user was idle. `screen_was_locked` should be `true` if the
  // screen was locked, otherwise `false`. NOTE: This should not be called on
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

  // Invoked when the user solves an adaptive captch.
  void NotifyDidSolveAdaptiveCaptcha() override;

 private:
  brave_ads::AdsClientNotifier ads_client_notifier_;

  mojo::PendingReceiver<bat_ads::mojom::BatAdsClientNotifier> pending_receiver_;
  mojo::Receiver<bat_ads::mojom::BatAdsClientNotifier> receiver_{this};
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_CLIENT_NOTIFIER_IMPL_H_
