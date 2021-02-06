/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/user_activity_frequency_cap.h"

#include <stdint.h>

#include "base/time/time.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"
#include "bat/ads/internal/platform/platform_helper.h"
#include "bat/ads/internal/user_activity/user_activity.h"

namespace ads {

namespace {

double GetPointsForEventType(const UserActivityEventType event_type) {
  switch (event_type) {
    case UserActivityEventType::kOpenedNewOrFocusedOnExistingTab:
    case UserActivityEventType::kClosedTab:
    case UserActivityEventType::kPlayedMedia: {
      return 1.0;
    }

    case UserActivityEventType::kBrowserWindowDidBecomeActive:
    case UserActivityEventType::kBrowserWindowDidEnterBackground: {
      return 0.5;
    }
  }
}

}  // namespace

UserActivityFrequencyCap::UserActivityFrequencyCap() = default;

UserActivityFrequencyCap::~UserActivityFrequencyCap() = default;

bool UserActivityFrequencyCap::ShouldAllow() {
  const UserActivityEventHistoryMap history =
      UserActivity::Get()->get_history();
  if (!DoesRespectCap(history)) {
    return false;
  }

  return true;
}

std::string UserActivityFrequencyCap::get_last_message() const {
  return last_message_;
}

bool UserActivityFrequencyCap::DoesRespectCap(
    const UserActivityEventHistoryMap& history) {
  if (PlatformHelper::GetInstance()->IsMobile()) {
    return true;
  }

  const int64_t time_constraint = base::Time::kSecondsPerHour;

  double score = 0.0;

  for (const auto& item : history) {
    const UserActivityEventHistory user_activity_event_history = item.second;

    const int occurrences = OccurrencesForRollingTimeConstraint(
        user_activity_event_history, time_constraint);

    if (occurrences == 0) {
      continue;
    }

    const UserActivityEventType event_type = item.first;
    const double points = GetPointsForEventType(event_type);

    score += points * occurrences;
  }

  if (score < 2.0) {
    return false;
  }

  return true;
}

}  // namespace ads
