/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_INTERACTION_BROWSING_USER_ACTIVITY_MANAGER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_INTERACTION_BROWSING_USER_ACTIVITY_MANAGER_H_

#include <cstdint>

#include "bat/ads/internal/browser/browser_manager_observer.h"
#include "bat/ads/internal/tabs/tab_manager_observer.h"
#include "bat/ads/internal/user_interaction/browsing/user_activity_event_info_aliases.h"
#include "bat/ads/internal/user_interaction/browsing/user_activity_event_types.h"
#include "bat/ads/page_transition_types.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace ads {

struct TabInfo;

class UserActivityManager final : public BrowserManagerObserver,
                                  public TabManagerObserver {
 public:
  UserActivityManager();
  ~UserActivityManager() override;

  UserActivityManager(const UserActivityManager&) = delete;
  UserActivityManager& operator=(const UserActivityManager&) = delete;

  static UserActivityManager* GetInstance();

  static bool HasInstance();

  void RecordEvent(const UserActivityEventType event_type);
  void RecordEventForPageTransition(const int32_t type);

  UserActivityEventList GetHistoryForTimeWindow(
      const base::TimeDelta time_window) const;

 private:
  void RecordEventForPageTransition(const PageTransitionType type);

  // BrowserManagerObserver:
  void OnBrowserDidBecomeActive() override;
  void OnBrowserDidResignActive() override;
  void OnBrowserDidEnterForeground() override;
  void OnBrowserDidEnterBackground() override;

  // TabManagerObserver:
  void OnTabDidChangeFocus(const int32_t id) override;
  void OnTabDidChange(const TabInfo& tab) override;
  void OnDidOpenNewTab(const TabInfo& tab) override;
  void OnDidCloseTab(const int32_t id) override;
  void OnTabDidStartPlayingMedia(const int32_t id) override;
  void OnTabDidStopPlayingMedia(const int32_t id) override;

  UserActivityEventList history_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_INTERACTION_BROWSING_USER_ACTIVITY_MANAGER_H_
