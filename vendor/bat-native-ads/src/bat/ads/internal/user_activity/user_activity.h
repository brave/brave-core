/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_ACTIVITY_USER_ACTIVITY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_ACTIVITY_USER_ACTIVITY_H_

#include <cstdint>

#include "bat/ads/internal/user_activity/user_activity_event_info_aliases.h"
#include "bat/ads/internal/user_activity/user_activity_event_types.h"
#include "bat/ads/page_transition_types.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace ads {

const int kMaximumHistoryEntries = 3600;

class UserActivity final {
 public:
  UserActivity();
  ~UserActivity();

  UserActivity(const UserActivity&) = delete;
  UserActivity& operator=(const UserActivity&) = delete;

  static UserActivity* Get();

  static bool HasInstance();

  void RecordEvent(const UserActivityEventType event_type);
  void RecordEventForPageTransition(const PageTransitionType type);
  void RecordEventForPageTransitionFromInt(const int32_t type);

  UserActivityEventList GetHistoryForTimeWindow(
      const base::TimeDelta time_window) const;

 private:
  UserActivityEventList history_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_ACTIVITY_USER_ACTIVITY_H_
