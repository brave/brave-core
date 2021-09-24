/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_ACTIVITY_USER_ACTIVITY_EVENT_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_ACTIVITY_USER_ACTIVITY_EVENT_INFO_H_

#include "base/time/time.h"
#include "bat/ads/internal/user_activity/user_activity_event_types.h"

namespace ads {

struct UserActivityEventInfo;

struct UserActivityEventInfo final {
  UserActivityEventInfo();
  UserActivityEventInfo(const UserActivityEventInfo& info);
  ~UserActivityEventInfo();

  bool operator==(const UserActivityEventInfo& rhs) const;
  bool operator!=(const UserActivityEventInfo& rhs) const;

  UserActivityEventType type;
  base::Time created_at;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_ACTIVITY_USER_ACTIVITY_EVENT_INFO_H_
