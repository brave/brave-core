/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ATTENTION_USER_ACTIVITY_USER_ACTIVITY_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ATTENTION_USER_ACTIVITY_USER_ACTIVITY_MANAGER_H_

#include <cstdint>

#include "brave/components/brave_ads/core/internal/application_state/browser_manager_observer.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager_observer.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/page_transition_types.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_event_info.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_event_types.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier_observer.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

struct TabInfo;

class UserActivityManager final : public AdsClientNotifierObserver,
                                  public BrowserManagerObserver,
                                  public TabManagerObserver {
 public:
  UserActivityManager();

  UserActivityManager(const UserActivityManager&) = delete;
  UserActivityManager& operator=(const UserActivityManager&) = delete;

  ~UserActivityManager() override;

  static UserActivityManager& GetInstance();

  void RecordEvent(UserActivityEventType event_type);

  UserActivityEventList GetHistoryForTimeWindow(
      base::TimeDelta time_window) const;

 private:
  void RecordEventForPageTransition(PageTransitionType type);

  // AdsClientNotifierObserver:
  void OnNotifyDidInitializeAds() override;
  void OnNotifyUserGestureEventTriggered(int32_t type) override;

  // BrowserManagerObserver:
  void OnBrowserDidBecomeActive() override;
  void OnBrowserDidResignActive() override;
  void OnBrowserDidEnterForeground() override;
  void OnBrowserDidEnterBackground() override;

  // TabManagerObserver:
  void OnDidOpenNewTab(const TabInfo& tab) override;
  void OnTabDidChangeFocus(int32_t tab_id) override;
  void OnTabDidChange(const TabInfo& tab) override;
  void OnDidCloseTab(int32_t tab_id) override;
  void OnTabDidStartPlayingMedia(int32_t tab_id) override;
  void OnTabDidStopPlayingMedia(int32_t tab_id) override;

  UserActivityEventList user_activity_events_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ATTENTION_USER_ACTIVITY_USER_ACTIVITY_MANAGER_H_
