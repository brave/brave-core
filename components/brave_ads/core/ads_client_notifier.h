/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_ADS_CLIENT_NOTIFIER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_ADS_CLIENT_NOTIFIER_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/observer_list.h"
#include "brave/components/brave_ads/core/ads_client_notifier_observer.h"

class GURL;

namespace brave_ads {

class AdsClientNotifier {
 public:
  AdsClientNotifier();

  AdsClientNotifier(const AdsClientNotifier&) = delete;
  AdsClientNotifier& operator=(const AdsClientNotifier&) = delete;

  AdsClientNotifier(AdsClientNotifier&&) noexcept = delete;
  AdsClientNotifier& operator=(AdsClientNotifier&&) noexcept = delete;

  virtual ~AdsClientNotifier();

  void AddObserver(AdsClientNotifierObserver* observer);
  void RemoveObserver(AdsClientNotifierObserver* observer);

  // Called when the user changes the locale of their operating system. This
  // call is not required if the operating system restarts the browser when
  // changing the locale. |locale| should be specified in either
  // <ISO-639-1>-<ISO-3166-1> or <ISO-639-1>_<ISO-3166-1> format.
  void NotifyLocaleDidChange(const std::string& locale);

  // Invoked when a preference has changed for the specified |path|.
  void NotifyPrefDidChange(const std::string& path);

  // Invoked when a resource component has been updated.
  void NotifyDidUpdateResourceComponent(const std::string& id);

  // Invoked when the page for |tab_id| has loaded and the content is available
  // for analysis. |redirect_chain| containing a list of redirect URLs that
  // occurred on the way to the current page. The current page is the last one
  // in the list (so even when there's no redirect, there should be one entry in
  // the list). |text| containing the page content as text.
  void NotifyTabTextContentDidChange(int32_t tab_id,
                                     const std::vector<GURL>& redirect_chain,
                                     const std::string& text);

  // Invoked when the page for |tab_id| has loaded and the content is available
  // for analysis. |redirect_chain| containing a list of redirect URLs that
  // occurred on the way to the current page. The current page is the last one
  // in the list (so even when there's no redirect, there should be one entry in
  // the list). |html| containing the page content as HTML.
  void NotifyTabHtmlContentDidChange(int32_t tab_id,
                                     const std::vector<GURL>& redirect_chain,
                                     const std::string& html);

  // Invoked when media starts playing on a browser tab for the specified
  // |tab_id|.
  void NotifyTabDidStartPlayingMedia(int32_t tab_id);

  // Called when media stops playing on a browser tab for the specified
  // |tab_id|.
  void NotifyTabDidStopPlayingMedia(int32_t tab_id);

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
                          bool is_incognito);

  // Invoked when a browser tab with the specified |tab_id| is closed.
  void NotifyDidCloseTab(int32_t tab_id);

  // Called when a page navigation was initiated by a user gesture.
  // |page_transition_type| containing the page transition type, see enums for
  // |PageTransitionType|.
  virtual void NotifyUserGestureEventTriggered(int32_t page_transition_type);

  // Invoked when a user has been idle for the threshold set in
  // |prefs::kIdleTimeThreshold|. NOTE: This should not be called on mobile
  // devices.
  void NotifyUserDidBecomeIdle();

  // Called when a user is no longer idle. |idle_time| is the amount of time in
  // seconds that the user was idle. |screen_was_locked| should be |true| if the
  // screen was locked, otherwise |false|. NOTE: This should not be called on
  // mobile devices.
  void NotifyUserDidBecomeActive(base::TimeDelta idle_time,
                                 bool screen_was_locked);

  // Invoked when the browser did enter the foreground.
  void NotifyBrowserDidEnterForeground();

  // Invoked when the browser did enter the background.
  void NotifyBrowserDidEnterBackground();

  // Invoked when the browser did become active.
  void NotifyBrowserDidBecomeActive();

  // Invoked when the browser did resign active.
  void NotifyBrowserDidResignActive();

 private:
  base::ObserverList<AdsClientNotifierObserver> observers_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_ADS_CLIENT_NOTIFIER_H_
