/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_INTERACTION_USER_ACTIVITY_USER_ACTIVITY_MANAGER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_INTERACTION_USER_ACTIVITY_USER_ACTIVITY_MANAGER_H_

#include <cstdint>

#include "bat/ads/internal/browser/browser_manager_observer.h"
#include "bat/ads/internal/tabs/tab_manager_observer.h"
#include "bat/ads/internal/user_interaction/user_activity/user_activity_event_info.h"
#include "bat/ads/internal/user_interaction/user_activity/user_activity_event_types.h"
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

  UserActivityManager(const UserActivityManager& other) = delete;
  UserActivityManager& operator=(const UserActivityManager& other) = delete;

  UserActivityManager(UserActivityManager&& other) noexcept = delete;
  UserActivityManager& operator=(UserActivityManager&& other) noexcept = delete;

  ~UserActivityManager() override;

  static UserActivityManager* GetInstance();

  static bool HasInstance();

  void RecordEvent(UserActivityEventType event_type);
  void RecordEventForPageTransition(int32_t type);

  UserActivityEventList GetHistoryForTimeWindow(
      base::TimeDelta time_window) const;

 private:
  void RecordEventForPageTransition(PageTransitionType type);

  // BrowserManagerObserver:
  void OnBrowserDidBecomeActive() override;
  void OnBrowserDidResignActive() override;
  void OnBrowserDidEnterForeground() override;
  void OnBrowserDidEnterBackground() override;

  // TabManagerObserver:
  void OnTabDidChangeFocus(int32_t tab_id) override;
  void OnTabDidChange(const TabInfo& tab) override;
  void OnDidOpenNewTab(const TabInfo& tab) override;
  void OnDidCloseTab(int32_t tab_id) override;
  void OnTabDidStartPlayingMedia(int32_t tab_id) override;
  void OnTabDidStopPlayingMedia(int32_t tab_id) override;

  UserActivityEventList history_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_INTERACTION_USER_ACTIVITY_USER_ACTIVITY_MANAGER_H_
