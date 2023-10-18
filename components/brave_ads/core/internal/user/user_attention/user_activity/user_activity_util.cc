/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user/user_attention/user_activity/user_activity_util.h"

#include <vector>

#include "base/containers/adapters.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"

namespace brave_ads {

size_t GetNumberOfUserActivityEvents(const UserActivityEventList& events,
                                     UserActivityEventType event_type) {
  return base::ranges::count_if(
      events, [event_type](const UserActivityEventInfo& event) {
        return event.type == event_type;
      });
}

base::TimeDelta GetTimeSinceLastUserActivityEvent(
    const UserActivityEventList& events,
    UserActivityEventType event_type) {
  const auto iter = base::ranges::find(base::Reversed(events), event_type,
                                       &UserActivityEventInfo::type);

  if (iter == events.crend()) {
    return {};
  }

  return base::Time::Now() - iter->created_at;
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

    const std::string& event_sequence = value.at(0);
    if (event_sequence.length() % 2 != 0) {
      continue;
    }

    UserActivityTriggerInfo trigger;
    trigger.event_sequence = base::ToUpperASCII(event_sequence);
    if (!base::StringToDouble(value.at(1), &trigger.score)) {
      continue;
    }

    triggers.push_back(trigger);
  }

  return triggers;
}

}  // namespace brave_ads
