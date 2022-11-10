/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_interaction/idle_detection/idle_detection.h"

#include "base/time/time.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/diagnostics/entries/last_unidle_time_diagnostic_util.h"
#include "bat/ads/internal/user_interaction/idle_detection/idle_detection_util.h"

namespace ads {

IdleDetection::IdleDetection() {
  MaybeUpdateIdleTimeThreshold();

  AdsClientHelper::AddObserver(this);
}

IdleDetection::~IdleDetection() {
  AdsClientHelper::RemoveObserver(this);
}

///////////////////////////////////////////////////////////////////////////////

void IdleDetection::OnUserDidBecomeActive(const base::TimeDelta idle_time,
                                          const bool screen_was_locked) {
  BLOG(1, "User is active after " << idle_time);
  if (screen_was_locked) {
    BLOG(1, "Screen was locked before the user become active");
  }

  MaybeUpdateIdleTimeThreshold();

  SetLastUnIdleTimeDiagnosticEntry();
}

void IdleDetection::OnUserDidBecomeIdle() {
  BLOG(1, "User is idle");
}

}  // namespace ads
