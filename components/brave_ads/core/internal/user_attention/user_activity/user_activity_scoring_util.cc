/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_scoring_util.h"

#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_event_info.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_features.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_manager.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_scoring.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_trigger_info.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_util.h"

namespace brave_ads {

bool WasUserActive() {
  const UserActivityTriggerList triggers =
      ToUserActivityTriggers(user_activity::features::GetTriggers());

  const base::TimeDelta time_window = user_activity::features::GetTimeWindow();
  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(time_window);

  const double score = GetUserActivityScore(triggers, events);

  const double threshold = user_activity::features::GetThreshold();
  return score >= threshold;
}

}  // namespace brave_ads
