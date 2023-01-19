/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_INTERACTION_USER_ACTIVITY_USER_ACTIVITY_EVENT_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_INTERACTION_USER_ACTIVITY_USER_ACTIVITY_EVENT_INFO_H_

#include "base/containers/circular_deque.h"
#include "base/time/time.h"
#include "bat/ads/internal/user_interaction/user_activity/user_activity_event_types.h"

namespace ads {

struct UserActivityEventInfo final {
  UserActivityEventType type;
  base::Time created_at;
};

bool operator==(const UserActivityEventInfo& lhs,
                const UserActivityEventInfo& rhs);
bool operator!=(const UserActivityEventInfo& lhs,
                const UserActivityEventInfo& rhs);

using UserActivityEventList = base::circular_deque<UserActivityEventInfo>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_INTERACTION_USER_ACTIVITY_USER_ACTIVITY_EVENT_INFO_H_
