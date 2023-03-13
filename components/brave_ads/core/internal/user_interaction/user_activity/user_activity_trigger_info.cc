/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_interaction/user_activity/user_activity_trigger_info.h"

#include "brave/components/brave_ads/core/internal/common/numbers/number_util.h"

namespace brave_ads {

bool operator==(const UserActivityTriggerInfo& lhs,
                const UserActivityTriggerInfo& rhs) {
  return lhs.event_sequence == rhs.event_sequence &&
         DoubleEquals(lhs.score, rhs.score);
}

bool operator!=(const UserActivityTriggerInfo& lhs,
                const UserActivityTriggerInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
