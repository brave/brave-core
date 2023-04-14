/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_attention/idle_detection/idle_detection.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/last_unidle_time_diagnostic_util.h"
#include "brave/components/brave_ads/core/internal/user_attention/idle_detection/idle_detection_util.h"

namespace brave_ads {

IdleDetection::IdleDetection() {
  MaybeUpdateIdleTimeThreshold();

  AdsClientHelper::AddObserver(this);
}

IdleDetection::~IdleDetection() {
  AdsClientHelper::RemoveObserver(this);
}

///////////////////////////////////////////////////////////////////////////////

void IdleDetection::OnNotifyUserDidBecomeActive(const base::TimeDelta idle_time,
                                                const bool screen_was_locked) {
  BLOG(1, "User is active after " << idle_time);
  if (screen_was_locked) {
    BLOG(1, "Screen was locked before the user become active");
  }

  MaybeUpdateIdleTimeThreshold();

  SetLastUnIdleTimeDiagnosticEntry(base::Time::Now());
}

void IdleDetection::OnNotifyUserDidBecomeIdle() {
  BLOG(1, "User is idle");
}

}  // namespace brave_ads
