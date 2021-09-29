/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_activity/user_activity_scoring_util.h"

#include "bat/ads/internal/features/user_activity/user_activity_features.h"
#include "bat/ads/internal/user_activity/user_activity.h"
#include "bat/ads/internal/user_activity/user_activity_event_info.h"
#include "bat/ads/internal/user_activity/user_activity_scoring.h"
#include "bat/ads/internal/user_activity/user_activity_trigger_info_aliases.h"
#include "bat/ads/internal/user_activity/user_activity_util.h"

namespace ads {

bool WasUserActive() {
  const UserActivityTriggerList triggers =
      ToUserActivityTriggers(features::user_activity::GetTriggers());

  const base::TimeDelta time_window = features::user_activity::GetTimeWindow();
  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(time_window);

  const double score = GetUserActivityScore(triggers, events);

  const double threshold = features::user_activity::GetThreshold();
  if (score < threshold) {
    return false;
  }

  return true;
}

}  // namespace ads
