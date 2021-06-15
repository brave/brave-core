/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_activity/user_activity_scoring.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "bat/ads/internal/features/user_activity/user_activity_features.h"
#include "bat/ads/internal/user_activity/user_activity.h"
#include "bat/ads/internal/user_activity/user_activity_util.h"

namespace ads {

namespace {

UserActivityTriggers SortTriggers(const UserActivityTriggers& triggers) {
  UserActivityTriggers mutable_triggers = triggers;

  std::sort(mutable_triggers.begin(), mutable_triggers.end(),
            [](const UserActivityTriggerInfo& lhs,
               const UserActivityTriggerInfo& rhs) {
              return lhs.event_sequence.length() >
                         rhs.event_sequence.length() &&
                     lhs.score > rhs.score;
            });

  return mutable_triggers;
}

std::string EncodeEvents(const UserActivityEvents& events) {
  std::vector<UserActivityEventType> eligible_events;
  for (const auto& event : events) {
    eligible_events.push_back(event.type);
  }

  const std::string encoded_eligible_events =
      base::HexEncode(eligible_events.data(), eligible_events.size());

  return base::ToUpperASCII(encoded_eligible_events);
}

double CalculateScore(const UserActivityTriggers& triggers,
                      const std::string& events) {
  std::string mutable_events = events;

  double score = 0.0;

  for (const auto& trigger : triggers) {
    std::string::size_type pos = 0;

    for (;;) {
      pos = mutable_events.find(trigger.event_sequence, pos);
      if (pos == std::string::npos) {
        break;
      }

      if (pos % 2 != 0) {
        // Event sequences start on hex/byte character boundaries
        pos++;
        continue;
      }

      mutable_events.erase(pos, trigger.event_sequence.length());
      score += trigger.score;
    }
  }

  return score;
}

}  // namespace

double GetUserActivityScore(const UserActivityTriggers& triggers,
                            const UserActivityEvents& events) {
  if (triggers.empty() || events.empty()) {
    return 0.0;
  }

  UserActivityTriggers sorted_triggers = SortTriggers(triggers);

  const std::string encoded_events = EncodeEvents(events);

  return CalculateScore(sorted_triggers, encoded_events);
}

}  // namespace ads
