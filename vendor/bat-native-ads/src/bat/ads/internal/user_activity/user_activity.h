/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_USER_ACTIVITY_USER_ACTIVITY_H_
#define BAT_ADS_INTERNAL_USER_ACTIVITY_USER_ACTIVITY_H_

#include <stdint.h>

#include <deque>
#include <map>

namespace ads {

enum class UserActivityType {
  kOpenedNewOrFocusedOnExistingTab,
  kClosedTab,
  kStartedPlayingMedia,
  kBrowserWindowDidBecomeActive,
  kBrowserWindowDidEnterBackground
};

using UserActivityHistory = std::deque<uint64_t>;
using UserActivityHistoryMap = std::map<UserActivityType, UserActivityHistory>;

class UserActivity {
 public:
  UserActivity();

  ~UserActivity();

  void RecordActivityForType(
      const UserActivityType type);

  const UserActivityHistoryMap& get_history() const;

 private:
  UserActivityHistoryMap history_;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_USER_ACTIVITY_USER_ACTIVITY_H_
