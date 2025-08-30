/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MISC_METRICS_USAGE_CLOCK_H_
#define BRAVE_BROWSER_MISC_METRICS_USAGE_CLOCK_H_

#include <optional>

#include "base/time/time.h"
#include "base/timer/elapsed_timer.h"
#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"

namespace misc_metrics {

// A clock that advances when Chrome is in use.
//
// See metrics::DesktopSessionDurationTracker for how Chrome usage is tracked.
// If metrics::DesktopSessionDurationTracker isn't initialized before this, the
// clock will advance continuously, regardless of Chrome usage. This avoids
// forcing all tests that indirectly depend on this to initialize
// metrics::DesktopSessionDurationTracker.
class UsageClock : public metrics::DesktopSessionDurationTracker::Observer {
 public:
  UsageClock();

  UsageClock(const UsageClock&) = delete;
  UsageClock& operator=(const UsageClock&) = delete;

  ~UsageClock() override;

  // Returns the amount of Chrome usage time since this was instantiated.
  base::TimeDelta GetTotalUsageTime() const;

  // Returns true if Chrome is currently considered to be in use.
  bool IsInUse() const;

 private:
  // DesktopSessionDurationTracker::Observer:
  void OnSessionStarted(base::TimeTicks session_start) override;
  void OnSessionEnded(base::TimeDelta session_length,
                      base::TimeTicks session_end) override;

  // The total time elapsed in completed usage sessions. The duration of the
  // current usage session, if any, must be added to this to get the total usage
  // time of Chrome.
  base::TimeDelta usage_time_in_completed_sessions_;

  // Elapsed timer for the current session, or nullopt if not currently in a
  // session.
  std::optional<base::ElapsedTimer> current_session_elapsed_timer_;
};

}  // namespace misc_metrics

#endif  // BRAVE_BROWSER_MISC_METRICS_USAGE_CLOCK_H_
