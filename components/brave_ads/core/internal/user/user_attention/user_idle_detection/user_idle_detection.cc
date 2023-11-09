/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user/user_attention/user_idle_detection/user_idle_detection.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/last_unidle_time_diagnostic_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"

namespace brave_ads {

UserIdleDetection::UserIdleDetection() {
  AddAdsClientNotifierObserver(this);
}

UserIdleDetection::~UserIdleDetection() {
  RemoveAdsClientNotifierObserver(this);
}

///////////////////////////////////////////////////////////////////////////////

void UserIdleDetection::OnNotifyUserDidBecomeActive(
    const base::TimeDelta idle_time,
    const bool screen_was_locked) {
  if (!UserHasJoinedBraveRewards()) {
    return;
  }

  BLOG(1, "User is active after " << idle_time);
  if (screen_was_locked) {
    BLOG(1, "Screen was locked before the user become active");
  }

  SetLastUnIdleTimeDiagnosticEntry(base::Time::Now());
}

void UserIdleDetection::OnNotifyUserDidBecomeIdle() {
  if (!UserHasJoinedBraveRewards()) {
    return;
  }

  BLOG(1, "User is idle");
}

}  // namespace brave_ads
