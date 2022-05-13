/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_activity/user_activity_event_info.h"

#include "bat/ads/internal/number_util.h"

namespace ads {

UserActivityEventInfo::UserActivityEventInfo() = default;

UserActivityEventInfo::UserActivityEventInfo(
    const UserActivityEventInfo& info) = default;

UserActivityEventInfo::~UserActivityEventInfo() = default;

bool UserActivityEventInfo::operator==(const UserActivityEventInfo& rhs) const {
  return type == rhs.type &&
         DoubleEquals(created_at.ToDoubleT(), rhs.created_at.ToDoubleT());
}

bool UserActivityEventInfo::operator!=(const UserActivityEventInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
