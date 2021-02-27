/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_activity/user_activity_trigger_info.h"

namespace ads {

UserActivityTriggerInfo::UserActivityTriggerInfo() = default;

UserActivityTriggerInfo::UserActivityTriggerInfo(
    const UserActivityTriggerInfo& info) = default;

UserActivityTriggerInfo::~UserActivityTriggerInfo() = default;

bool UserActivityTriggerInfo::operator==(
    const UserActivityTriggerInfo& rhs) const {
  return event_sequence == rhs.event_sequence && score == rhs.score;
}

bool UserActivityTriggerInfo::operator!=(
    const UserActivityTriggerInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
