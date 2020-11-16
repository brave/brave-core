/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/user_activity_frequency_cap.h"

#include <stdint.h>

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"
#include "bat/ads/internal/platform/platform_helper.h"
#include "bat/ads/internal/time_util.h"

namespace ads {

UserActivityFrequencyCap::UserActivityFrequencyCap(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

UserActivityFrequencyCap::~UserActivityFrequencyCap() = default;

bool UserActivityFrequencyCap::ShouldAllow() {
  const UserActivityHistoryMap history =
      ads_->get_user_activity()->get_history();
  if (!DoesRespectCap(history)) {
    return false;
  }

  return true;
}

std::string UserActivityFrequencyCap::get_last_message() const {
  return last_message_;
}

bool UserActivityFrequencyCap::DoesRespectCap(
    const UserActivityHistoryMap& history) {
  if (PlatformHelper::GetInstance()->IsMobile()) {
    return true;
  }

  const uint64_t time_constraint = base::Time::kSecondsPerHour;

  double total_score = 0.0;

  for (const auto& item : history) {
    const UserActivityHistory user_activity_history = item.second;

    const uint64_t occurrences = OccurrencesForRollingTimeConstraint(
        user_activity_history, time_constraint);

    if (occurrences == 0) {
      continue;
    }

    double score;

    const UserActivityType user_activity_type = item.first;

    switch (user_activity_type) {
      case UserActivityType::kOpenedNewOrFocusedOnExistingTab:
      case UserActivityType::kClosedTab:
      case UserActivityType::kStartedPlayingMedia: {
        score = 1.0;
        break;
      }

      case UserActivityType::kBrowserWindowDidBecomeActive:
      case UserActivityType::kBrowserWindowDidEnterBackground: {
        score = 0.5;
        break;
      }
    }

    total_score += score * occurrences;
  }

  if (total_score < 2.0) {
    return false;
  }

  return true;
}

}  // namespace ads
