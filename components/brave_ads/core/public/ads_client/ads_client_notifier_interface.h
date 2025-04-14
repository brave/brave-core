/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_CLIENT_ADS_CLIENT_NOTIFIER_INTERFACE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_CLIENT_ADS_CLIENT_NOTIFIER_INTERFACE_H_

#include <cstdint>
#include <string>
#include <vector>

class GURL;

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

class AdsClientNotifierObserver;

class AdsClientNotifierInterface {
 public:
  // Invoked to add an observer.
  virtual void AddObserver(AdsClientNotifierObserver* observer) = 0;

  // Invoked to remove an observer.
  virtual void RemoveObserver(AdsClientNotifierObserver* observer) = 0;

  // Invoked to fire all pending observer events.
  virtual void NotifyPendingObservers() = 0;

  // Invoked when ads did initialize.
  virtual void NotifyDidInitializeAds() = 0;

  // Invoked when the user changes the locale of their operating system. This
  // call is not required if the operating system restarts the browser when
  // changing the locale. `locale` should be specified in either
  // <ISO-639-1>-<ISO-3166-1> or <ISO-639-1>_<ISO-3166-1> format.
  virtual void NotifyLocaleDidChange(const std::string& locale) = 0;

  // Invoked when a preference has changed for the specified `path`.
  virtual void NotifyPrefDidChange(const std::string& path) = 0;

  // Invoked when a resource component with `id` has been updated to
  // `manifest_version`.
  virtual void NotifyResourceComponentDidChange(
      const std::string& manifest_version,
      const std::string& id) = 0;

  // Invoked when a resource component with `id` has been unregistered.
  virtual void NotifyDidUnregisterResourceComponent(const std::string& id) = 0;

  // Invoked when the Brave Rewards wallet did update.
  virtual void NotifyRewardsWalletDidUpdate(
      const std::string& payment_id,
      const std::string& recovery_seed_base64) = 0;

  // Invoked when the page for `tab_id` has loaded and the content is available
  // for analysis. `redirect_chain` containing a list of redirect URLs that
  // occurred on the way to the current page. The current page is the last one
  // in the list (so even when there's no redirect, there should be one entry in
  // the list). `text` containing the page content as text.
  virtual void NotifyTabTextContentDidChange(
      int32_t tab_id,
      const std::vector<GURL>& redirect_chain,
      const std::string& text) = 0;

  // Invoked when the page for `tab_id` has loaded and the content is available
  // for analysis. `redirect_chain` containing a list of redirect URLs that
  // occurred on the way to the current page. The current page is the last one
  // in the list (so even when there's no redirect, there should be one entry in
  // the list). `html` containing the page content as HTML.
  virtual void NotifyTabHtmlContentDidChange(
      int32_t tab_id,
      const std::vector<GURL>& redirect_chain,
      const std::string& html) = 0;

  // Invoked when media starts playing on a browser tab for the specified
  // `tab_id`.
  virtual void NotifyTabDidStartPlayingMedia(int32_t tab_id) = 0;

  // Invoked when media stops playing on a browser tab for the specified
  // `tab_id`.
  virtual void NotifyTabDidStopPlayingMedia(int32_t tab_id) = 0;

  // Invoked when a browser tab is updated with the specified `redirect_chain`
  // containing a list of redirect URLs that occurred on the way to the current
  // page. The current page is the last one in the list (so even when there's no
  // redirect, there should be one entry in the list). `is_restoring` should be
  // set to `true` if the page is restoring otherwise should be set to `false`.
  // `is_visible` should be set to `true` if `tab_id` refers to the currently
  // visible tab otherwise should be set to `false`.
  virtual void NotifyTabDidChange(int32_t tab_id,
                                  const std::vector<GURL>& redirect_chain,
                                  bool is_new_navigation,
                                  bool is_restoring,
                                  bool is_visible) = 0;

  // Invoked when a browser tab has loaded. `http_status_code` should be set to
  // the HTTP response code.
  virtual void NotifyTabDidLoad(int32_t tab_id, int http_status_code) = 0;

  // Invoked when a browser tab with the specified `tab_id` is closed.
  virtual void NotifyDidCloseTab(int32_t tab_id) = 0;

  // Invoked when a page navigation was initiated by a user gesture.
  // `page_transition_type` containing the page transition type, see enums for
  // `PageTransitionType`.
  virtual void NotifyUserGestureEventTriggered(
      int32_t page_transition_type) = 0;

  // Invoked when a user has been idle for the given threshold. NOTE: This
  // should not be called on mobile devices.
  virtual void NotifyUserDidBecomeIdle() = 0;

  // Invoked when a user is no longer idle. `idle_time` is the duration of time
  // that the user was idle. `screen_was_locked` should be `true` if the screen
  // was locked, otherwise `false`. NOTE: This should not be called on mobile
  // devices.
  virtual void NotifyUserDidBecomeActive(base::TimeDelta idle_time,
                                         bool screen_was_locked) = 0;

  // Invoked when the browser did enter the foreground.
  virtual void NotifyBrowserDidEnterForeground() = 0;

  // Invoked when the browser did enter the background.
  virtual void NotifyBrowserDidEnterBackground() = 0;

  // Invoked when the browser did become active.
  virtual void NotifyBrowserDidBecomeActive() = 0;

  // Invoked when the browser did resign active.
  virtual void NotifyBrowserDidResignActive() = 0;

  // Invoked when the user solves an adaptive captcha.
  virtual void NotifyDidSolveAdaptiveCaptcha() = 0;

 protected:
  virtual ~AdsClientNotifierInterface() = default;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_CLIENT_ADS_CLIENT_NOTIFIER_INTERFACE_H_
