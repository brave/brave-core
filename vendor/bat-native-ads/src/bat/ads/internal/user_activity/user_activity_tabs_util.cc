/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_activity/user_activity_tabs_util.h"

#include "bat/ads/internal/user_activity/user_activity_event_types.h"

namespace ads {

int GetNumberOfTabsOpened(const UserActivityEventList& events) {
  const int count = std::count_if(
      events.cbegin(), events.cend(), [](const UserActivityEventInfo& event) {
        return event.type == UserActivityEventType::kOpenedNewTab;
      });

  return count;
}

}  // namespace ads
