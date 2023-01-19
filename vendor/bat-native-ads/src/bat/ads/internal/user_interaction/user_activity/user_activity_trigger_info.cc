/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_interaction/user_activity/user_activity_trigger_info.h"

#include "bat/ads/internal/common/numbers/number_util.h"

namespace ads {

bool operator==(const UserActivityTriggerInfo& lhs,
                const UserActivityTriggerInfo& rhs) {
  return lhs.event_sequence == rhs.event_sequence &&
         DoubleEquals(lhs.score, rhs.score);
}

bool operator!=(const UserActivityTriggerInfo& lhs,
                const UserActivityTriggerInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace ads
