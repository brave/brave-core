/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_INTERACTION_USER_ACTIVITY_USER_ACTIVITY_TRIGGER_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_INTERACTION_USER_ACTIVITY_USER_ACTIVITY_TRIGGER_INFO_H_

#include <string>
#include <vector>

namespace ads {

struct UserActivityTriggerInfo final {
  std::string event_sequence;
  double score = 0.0;
};

bool operator==(const UserActivityTriggerInfo& lhs,
                const UserActivityTriggerInfo& rhs);
bool operator!=(const UserActivityTriggerInfo& lhs,
                const UserActivityTriggerInfo& rhs);

using UserActivityTriggerList = std::vector<UserActivityTriggerInfo>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_INTERACTION_USER_ACTIVITY_USER_ACTIVITY_TRIGGER_INFO_H_
