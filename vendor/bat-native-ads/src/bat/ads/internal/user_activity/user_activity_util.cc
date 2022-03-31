/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_activity/user_activity_util.h"

#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "bat/ads/internal/user_activity/user_activity_trigger_info.h"

namespace ads {

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
