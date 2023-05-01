/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_event_info.h"

namespace brave_ads {

bool operator==(const UserActivityEventInfo& lhs,
                const UserActivityEventInfo& rhs) {
  return lhs.type == rhs.type && lhs.created_at == rhs.created_at;
}

bool operator!=(const UserActivityEventInfo& lhs,
                const UserActivityEventInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
