/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_interaction/idle_detection/idle_detection_util.h"

#include "base/time/time.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/user_interaction/user_activity/user_activity_features.h"
#include "brave/components/brave_ads/common/pref_names.h"

namespace ads {

bool MaybeScreenWasLocked(const bool screen_was_locked) {
  return user_activity::features::ShouldDetectScreenWasLocked() &&
         screen_was_locked;
}

bool HasExceededMaximumIdleTime(const base::TimeDelta idle_time) {
  const base::TimeDelta maximum_idle_time =
      user_activity::features::GetMaximumIdleTime();
  if (maximum_idle_time.is_zero()) {  // Infinite
    return false;
  }

  return idle_time > maximum_idle_time;
}

bool MaybeUpdateIdleTimeThreshold() {
  const int last_idle_time_threshold =
      AdsClientHelper::GetInstance()->GetIntegerPref(prefs::kIdleTimeThreshold);

  const base::TimeDelta idle_time_threshold =
      user_activity::features::GetIdleTimeThreshold();

  const int idle_time_threshold_as_int =
      static_cast<int>(idle_time_threshold.InSeconds());
  if (idle_time_threshold_as_int == last_idle_time_threshold) {
    return false;
  }

  AdsClientHelper::GetInstance()->SetIntegerPref(prefs::kIdleTimeThreshold,
                                                 idle_time_threshold_as_int);

  return true;
}

}  // namespace ads
