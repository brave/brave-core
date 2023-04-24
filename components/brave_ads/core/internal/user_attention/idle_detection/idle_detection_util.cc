/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_attention/idle_detection/idle_detection_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/user_attention/idle_detection/idle_detection_feature.h"

namespace brave_ads {

bool MaybeScreenWasLocked(const bool screen_was_locked) {
  return kShouldDetectScreenWasLocked.Get() && screen_was_locked;
}

bool HasExceededMaximumIdleTime(const base::TimeDelta idle_time) {
  const base::TimeDelta maximum_idle_time = kMaximumIdleTime.Get();
  if (maximum_idle_time.is_zero()) {  // Infinite
    return false;
  }

  return idle_time > maximum_idle_time;
}

bool MaybeUpdateIdleTimeThreshold() {
  const int last_idle_time_threshold =
      AdsClientHelper::GetInstance()->GetIntegerPref(prefs::kIdleTimeThreshold);

  const base::TimeDelta idle_time_threshold = kIdleTimeThreshold.Get();

  const int idle_time_threshold_as_int =
      static_cast<int>(idle_time_threshold.InSeconds());
  if (idle_time_threshold_as_int == last_idle_time_threshold) {
    return false;
  }

  AdsClientHelper::GetInstance()->SetIntegerPref(prefs::kIdleTimeThreshold,
                                                 idle_time_threshold_as_int);

  return true;
}

}  // namespace brave_ads
