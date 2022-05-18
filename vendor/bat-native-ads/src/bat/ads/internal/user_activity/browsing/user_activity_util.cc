/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_activity/browsing/user_activity_util.h"

#include <algorithm>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "bat/ads/internal/user_activity/browsing/user_activity_trigger_info.h"

namespace ads {

int GetNumberOfTabsOpened(const UserActivityEventList& events) {
  return std::count_if(
      events.cbegin(), events.cend(), [](const UserActivityEventInfo& event) {
        return event.type == UserActivityEventType::kOpenedNewTab;
      });
}

int GetNumberOfUserActivityEvents(const UserActivityEventList& events,
                                  UserActivityEventType event_type) {
  return std::count_if(events.cbegin(), events.cend(),
                       [event_type](const UserActivityEventInfo& event) {
                         return event.type == event_type;
                       });
}

int64_t GetTimeSinceLastUserActivityEvent(const UserActivityEventList& events,
                                          UserActivityEventType event_type) {
  const auto iter =
      std::find_if(events.crbegin(), events.crend(),
                   [event_type](const UserActivityEventInfo& event) {
                     return event.type == event_type;
                   });

  if (iter == events.crend()) {
    return kUserActivityMissingValue;
  }

  const base::TimeDelta time_delta = base::Time::Now() - iter->created_at;
  return time_delta.InSeconds();
}

UserActivityTriggerList ToUserActivityTriggers(const std::string& param_value) {
  UserActivityTriggerList triggers;

  const std::vector<std::string> components = base::SplitString(
      param_value, ";", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  for (const auto& component : components) {
    const std::vector<std::string> value = base::SplitString(
        component, "=", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

    if (value.size() != 2) {
      continue;
    }

    const std::string event_sequence = value.at(0);
    if (event_sequence.length() % 2 != 0) {
      continue;
    }

    UserActivityTriggerInfo trigger;
    trigger.event_sequence = base::ToUpperASCII(event_sequence);
    base::StringToDouble(value.at(1), &trigger.score);

    triggers.push_back(trigger);
  }

  return triggers;
}

}  // namespace ads
