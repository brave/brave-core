/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/usage_clock.h"

#include "base/check.h"

namespace misc_metrics {

UsageClock::UsageClock() {
  bool in_session = true;
  if (metrics::DesktopSessionDurationTracker::IsInitialized()) {
    auto* tracker = metrics::DesktopSessionDurationTracker::Get();
    tracker->AddObserver(this);
    if (!tracker->in_session()) {
      in_session = false;
    }
  }
  if (in_session) {
    current_session_elapsed_timer_ = base::ElapsedTimer();
  }
}

UsageClock::~UsageClock() {
  if (metrics::DesktopSessionDurationTracker::IsInitialized()) {
    metrics::DesktopSessionDurationTracker::Get()->RemoveObserver(this);
  }
}

base::TimeDelta UsageClock::GetTotalUsageTime() const {
  base::TimeDelta elapsed_time_in_session = usage_time_in_completed_sessions_;
  if (current_session_elapsed_timer_) {
    elapsed_time_in_session += current_session_elapsed_timer_->Elapsed();
  }
  return elapsed_time_in_session;
}

bool UsageClock::IsInUse() const {
  return current_session_elapsed_timer_.has_value();
}

void UsageClock::OnSessionStarted(base::TimeTicks session_start) {
  // Ignore |session_start| because it doesn't come from the resource
  // coordinator clock.
  DCHECK(!IsInUse());
  current_session_elapsed_timer_ = base::ElapsedTimer();
}

void UsageClock::OnSessionEnded(base::TimeDelta session_length,
                                base::TimeTicks session_end) {
  // Ignore |session_length| because it wasn't measured using the resource
  // coordinator clock.
  DCHECK(IsInUse());
  usage_time_in_completed_sessions_ +=
      current_session_elapsed_timer_->Elapsed();
  current_session_elapsed_timer_ = std::nullopt;
}

}  // namespace misc_metrics
