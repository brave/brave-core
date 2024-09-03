/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_CLIENT_ADS_CLIENT_NOTIFIER_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_CLIENT_ADS_CLIENT_NOTIFIER_OBSERVER_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "url/gurl.h"

namespace brave_ads {

class AdsClientNotifierObserver : public base::CheckedObserver {
 public:
  AdsClientNotifierObserver();

  AdsClientNotifierObserver(const AdsClientNotifierObserver&) = delete;
  AdsClientNotifierObserver& operator=(const AdsClientNotifierObserver&) =
      delete;

  AdsClientNotifierObserver(AdsClientNotifierObserver&&) noexcept = delete;
  AdsClientNotifierObserver& operator=(AdsClientNotifierObserver&&) noexcept =
      delete;

  ~AdsClientNotifierObserver() override;

  // Called when ads did initialize.
  virtual void OnNotifyDidInitializeAds() {}

  // Called when the user changes the locale of their operating system. This
  // call is not required if the operating system restarts the browser when
  // changing the locale. `locale` should be specified in either
  // <ISO-639-1>-<ISO-3166-1> or <ISO-639-1>_<ISO-3166-1> format.
  virtual void OnNotifyLocaleDidChange(const std::string& locale) {}

  // Invoked when a preference has changed for the specified `path`.
  virtual void OnNotifyPrefDidChange(const std::string& path) {}

  // Invoked when a resource component with `id` has been updated to
  // `manifest_version`.
  virtual void OnNotifyResourceComponentDidChange(
      const std::string& manifest_version,
      const std::string& id) {}

  // Invoked when a resource component with `id` has been unregistered.
  virtual void OnNotifyDidUnregisterResourceComponent(const std::string& id) {}

  // Called when the Brave Rewards wallet did update.
  virtual void OnNotifyRewardsWalletDidUpdate(
      const std::string& payment_id,
      const std::string& recovery_seed_base64) {}

  // Invoked when the page for `tab_id` has loaded and the content is available
  // for analysis. `redirect_chain` containing a list of redirect URLs that
  // occurred on the way to the current page. The current page is the last one
  // in the list (so even when there's no redirect, there should be one entry in
  // the list). `text` containing the page content as text.
  virtual void OnNotifyTabTextContentDidChange(
      int32_t tab_id,
      const std::vector<GURL>& redirect_chain,
      const std::string& text) {}

  // Invoked when the page for `tab_id` has loaded and the content is available
  // for analysis. `redirect_chain` containing a list of redirect URLs that
  // occurred on the way to the current page. The current page is the last one
  // in the list (so even when there's no redirect, there should be one entry in
  // the list). `html` containing the page content as HTML.
  virtual void OnNotifyTabHtmlContentDidChange(
      int32_t tab_id,
      const std::vector<GURL>& redirect_chain,
      const std::string& html) {}

  // Invoked when media starts playing on a browser tab for the specified
  // `tab_id`.
  virtual void OnNotifyTabDidStartPlayingMedia(int32_t tab_id) {}

  // Called when media stops playing on a browser tab for the specified
  // `tab_id`.
  virtual void OnNotifyTabDidStopPlayingMedia(int32_t tab_id) {}

  // Invoked when a browser tab is updated with the specified `redirect_chain`
  // containing a list of redirect URLs that occurred on the way to the current
  // page. The current page is the last one in the list (so even when there's no
  // redirect, there should be one entry in the list). `is_restoring` should be
  // set to `true` if the page is restoring otherwise should be set to `false`.
  // `is_error_page` should be set to `true` if an error occurred otherwise
  // should be set to `false`. `is_visible` should be set to `true` if `tab_id`
  // refers to the currently visible tab otherwise should be set to `false`.
  virtual void OnNotifyTabDidChange(int32_t tab_id,
                                    const std::vector<GURL>& redirect_chain,
                                    const bool is_new_navigation,
                                    bool is_restoring,
                                    bool is_error_page,
                                    bool is_visible) {}

  // Invoked when a browser tab with the specified `tab_id` is closed.
  virtual void OnNotifyDidCloseTab(int32_t tab_id) {}

  // Called when a page navigation was initiated by a user gesture.
  // `page_transition_type` containing the page transition type, see enums for
  // `PageTransitionType`.
  virtual void OnNotifyUserGestureEventTriggered(int32_t page_transition_type) {
  }

  // Invoked when a user has been idle for the given threshold. NOTE: This
  // should not be called on mobile devices.
  virtual void OnNotifyUserDidBecomeIdle() {}

  // Called when a user is no longer idle. `idle_time` is the amount of time in
  // seconds that the user was idle. `screen_was_locked` should be `true` if the
  // screen was locked, otherwise `false`. NOTE: This should not be called on
  // mobile devices.
  virtual void OnNotifyUserDidBecomeActive(base::TimeDelta idle_time,
                                           bool screen_was_locked) {}

  // Invoked when the browser did enter the foreground.
  virtual void OnNotifyBrowserDidEnterForeground() {}

  // Invoked when the browser did enter the background.
  virtual void OnNotifyBrowserDidEnterBackground() {}

  // Invoked when the browser did become active.
  virtual void OnNotifyBrowserDidBecomeActive() {}

  // Invoked when the browser did resign active.
  virtual void OnNotifyBrowserDidResignActive() {}

  // Invoked when the user solves an adaptive captcha.
  virtual void OnNotifyDidSolveAdaptiveCaptcha() {}
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_CLIENT_ADS_CLIENT_NOTIFIER_OBSERVER_H_
