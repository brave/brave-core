/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_CLIENT_ADS_CLIENT_NOTIFIER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_CLIENT_ADS_CLIENT_NOTIFIER_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "brave/components/brave_ads/core/public/client/ads_client_notifier_observer.h"

class GURL;

namespace brave_ads {

class AdsClientNotifierQueue;

class AdsClientNotifier {
 public:
  AdsClientNotifier();

  AdsClientNotifier(const AdsClientNotifier&) = delete;
  AdsClientNotifier& operator=(const AdsClientNotifier&) = delete;

  AdsClientNotifier(AdsClientNotifier&&) noexcept = delete;
  AdsClientNotifier& operator=(AdsClientNotifier&&) noexcept = delete;

  virtual ~AdsClientNotifier();

  void set_should_queue_notifications_for_testing(
      bool should_queue_notifications) {
    should_queue_notifications_ = should_queue_notifications;
  }

  void AddObserver(AdsClientNotifierObserver* observer);
  void RemoveObserver(AdsClientNotifierObserver* observer);

  // Invoked to fire all pending observer events.
  void NotifyPendingObservers();

  // Invoked when ads did initialize.
  void NotifyDidInitializeAds() const;

  // Invoked when the user changes the locale of their operating system. This
  // call is not required if the operating system restarts the browser when
  // changing the locale. `locale` should be specified in either
  // <ISO-639-1>-<ISO-3166-1> or <ISO-639-1>_<ISO-3166-1> format.
  void NotifyLocaleDidChange(const std::string& locale) const;

  // Invoked when a preference has changed for the specified `path`.
  void NotifyPrefDidChange(const std::string& path) const;

  // Invoked when a resource component with `id` has been updated to
  // `manifest_version`.
  void NotifyDidUpdateResourceComponent(const std::string& manifest_version,
                                        const std::string& id) const;

  // Invoked when a resource component with `id` has been unregistered.
  void NotifyDidUnregisterResourceComponent(const std::string& id) const;

  // Invoked when the Brave Rewards wallet did update.
  void NotifyRewardsWalletDidUpdate(const std::string& payment_id,
                                    const std::string& recovery_seed) const;

  // Invoked when the page for `tab_id` has loaded and the content is available
  // for analysis. `redirect_chain` containing a list of redirect URLs that
  // occurred on the way to the current page. The current page is the last one
  // in the list (so even when there's no redirect, there should be one entry in
  // the list). `text` containing the page content as text.
  void NotifyTabTextContentDidChange(int32_t tab_id,
                                     const std::vector<GURL>& redirect_chain,
                                     const std::string& text) const;

  // Invoked when the page for `tab_id` has loaded and the content is available
  // for analysis. `redirect_chain` containing a list of redirect URLs that
  // occurred on the way to the current page. The current page is the last one
  // in the list (so even when there's no redirect, there should be one entry in
  // the list). `html` containing the page content as HTML.
  void NotifyTabHtmlContentDidChange(int32_t tab_id,
                                     const std::vector<GURL>& redirect_chain,
                                     const std::string& html) const;

  // Invoked when media starts playing on a browser tab for the specified
  // `tab_id`.
  void NotifyTabDidStartPlayingMedia(int32_t tab_id) const;

  // Invoked when media stops playing on a browser tab for the specified
  // `tab_id`.
  void NotifyTabDidStopPlayingMedia(int32_t tab_id) const;

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
                          bool is_visible) const;

  // Invoked when a browser tab with the specified `tab_id` is closed.
  void NotifyDidCloseTab(int32_t tab_id) const;

  // Invoked when a page navigation was initiated by a user gesture.
  // `page_transition_type` containing the page transition type, see enums for
  // `PageTransitionType`.
  void NotifyUserGestureEventTriggered(int32_t page_transition_type) const;

  // Invoked when a user has been idle for the given threshold. NOTE: This
  // should not be called on mobile devices.
  void NotifyUserDidBecomeIdle() const;

  // Invoked when a user is no longer idle. `idle_time` is the duration of time
  // that the user was idle. `screen_was_locked` should be `true` if the screen
  // was locked, otherwise `false`. NOTE: This should not be called on mobile
  // devices.
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

  // Invoked when the user solves an adaptive captch.
  void NotifyDidSolveAdaptiveCaptcha() const;

 private:
  base::ObserverList<AdsClientNotifierObserver> observers_;

  std::unique_ptr<AdsClientNotifierQueue> pending_notifier_queue_;

#if BUILDFLAG(IS_IOS)
  bool should_queue_notifications_ = true;
#else
  bool should_queue_notifications_ = false;
#endif  // BUILDFLAG(IS_IOS)

  base::WeakPtrFactory<AdsClientNotifier> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_CLIENT_ADS_CLIENT_NOTIFIER_H_
