/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_ACTIVITY_USER_ACTIVITY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_ACTIVITY_USER_ACTIVITY_H_

#include <stdint.h>

#include <deque>
#include <map>

namespace ads {

enum class UserActivityEventType {
  kOpenedNewOrFocusedOnExistingTab,
  kClosedTab,
  kPlayedMedia,
  kBrowserWindowDidBecomeActive,
  kBrowserWindowDidEnterBackground
};

using UserActivityEventHistory = std::deque<int64_t>;
using UserActivityEventHistoryMap =
    std::map<UserActivityEventType, UserActivityEventHistory>;

class UserActivity {
 public:
  UserActivity();

  ~UserActivity();

  UserActivity(const UserActivity&) = delete;
  UserActivity& operator=(const UserActivity&) = delete;

  static UserActivity* Get();

  static bool HasInstance();

  void RecordEvent(const UserActivityEventType event);

  const UserActivityEventHistoryMap& get_history() const;

 private:
  UserActivityEventHistoryMap history_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_ACTIVITY_USER_ACTIVITY_H_
