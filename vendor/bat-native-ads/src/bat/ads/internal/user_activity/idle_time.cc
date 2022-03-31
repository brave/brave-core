/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_activity/idle_time.h"

#include "base/time/time.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/user_activity/user_activity_features.h"
#include "bat/ads/pref_names.h"

namespace ads {

bool WasLocked(const bool was_locked) {
  return features::user_activity::ShouldDetectWasLocked() && was_locked;
}

bool HasExceededMaximumIdleTime(const int idle_time) {
  const base::TimeDelta maximum_idle_time =
      features::user_activity::GetMaximumIdleTime();

  const int seconds = static_cast<int>(maximum_idle_time.InSeconds());

  if (seconds == 0) {  // Infinite
    return false;
  }

  return idle_time > seconds;
}

bool MaybeUpdateIdleTimeThreshold() {
  const int last_idle_time_threshold =
      AdsClientHelper::Get()->GetIntegerPref(prefs::kIdleTimeThreshold);

  const base::TimeDelta idle_time_threshold =
      features::user_activity::GetIdleTimeThreshold();

  const int idle_time_threshold_as_int =
      static_cast<int>(idle_time_threshold.InSeconds());
  if (idle_time_threshold_as_int == last_idle_time_threshold) {
    return false;
  }

  AdsClientHelper::Get()->SetIntegerPref(prefs::kIdleTimeThreshold,
                                         idle_time_threshold_as_int);

  return true;
}

}  // namespace ads
